/*
 * domain_disc_impl.h
 *
 *  Created on: 29.06.2010
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__SPATIAL_DISC__DOMAIN_DISC_IMPL__
#define __H__UG__LIB_DISC__SPATIAL_DISC__DOMAIN_DISC_IMPL__

#include "domain_disc.h"
#include "lib_disc/common/groups_util.h"
#include "lib_disc/spatial_disc/elem_disc/elem_disc_assemble_util.h"
#ifdef UG_PARALLEL
#include "lib_disc/parallelization/parallelization_util.h"
#endif

namespace ug{

template <typename TDomain, typename TAlgebra>
void DomainDiscretization<TDomain, TAlgebra>::update_elem_discs()
{
//	check Approximation space
	if(!m_spApproxSpace.valid())
		UG_THROW("DomainDiscretization: Before using the "
				"DomainDiscretization an ApproximationSpace must be set to it. "
				"Please use DomainDiscretization:set_approximation_space to "
				"set an appropriate Space.");

//	set approximation space and extract IElemDiscs
	m_vElemDisc.clear();
	for(size_t i = 0; i < m_vDomainElemDisc.size(); ++i)
	{
		m_vDomainElemDisc[i]->set_approximation_space(m_spApproxSpace);

		if(!(m_vDomainElemDisc[i]->type() & m_ElemTypesEnabled)) continue;
		m_vElemDisc.push_back(m_vDomainElemDisc[i].get());
	}
}

template <typename TDomain, typename TAlgebra>
void DomainDiscretization<TDomain, TAlgebra>::update_constraints()
{
//	check Approximation space
	if(!m_spApproxSpace.valid())
		UG_THROW("DomainDiscretization: Before using the "
				"DomainDiscretization an ApproximationSpace must be set to it. "
				"Please use DomainDiscretization:set_approximation_space to "
				"set an appropriate Space.");


	for(size_t i = 0; i < m_vConstraint.size(); ++i)
		m_vConstraint[i]->set_approximation_space(m_spApproxSpace);
}

template <typename TDomain, typename TAlgebra>
void DomainDiscretization<TDomain, TAlgebra>::update_disc_items()
{
	update_elem_discs();
	update_constraints();
}


///////////////////////////////////////////////////////////////////////////////
// Mass Matrix
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
assemble_mass_matrix(matrix_type& M, const vector_type& u,
                     ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	reset matrix to zero and resize
	const size_t numIndex = dd->num_indices();
	M.resize(0,0);
	M.resize(numIndex, numIndex);
	M.set(0.0);

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			AssembleMassMatrix<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, M, u, m_pBoolMarker);
			break;
		case 2:
			AssembleMassMatrix<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, M, u, m_pBoolMarker);
			AssembleMassMatrix<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, M, u, m_pBoolMarker);
			break;
		case 3:
			AssembleMassMatrix<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, M, u, m_pBoolMarker);
			AssembleMassMatrix<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, M, u, m_pBoolMarker);
			AssembleMassMatrix<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, M, u, m_pBoolMarker);
			AssembleMassMatrix<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, M, u, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::assemble_mass_matrix:"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::assemble_mass_matrix:"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}

//	post process
	try{
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_jacobian(M, u, dd->grid_level());
	}
	}UG_CATCH_THROW("DomainDiscretization::assemble_mass_matrix:"
					" Cannot execute post process.");

//	Remember parallel storage type
#ifdef UG_PARALLEL
	M.set_storage_type(PST_ADDITIVE);
	TDD* pDD = const_cast<TDD*>(dd.get());
	CopyLayoutsAndCommunicatorIntoMatrix(M, *pDD);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Stiffness Matrix
///////////////////////////////////////////////////////////////////////////////

template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
assemble_stiffness_matrix(matrix_type& A, const vector_type& u,
                          ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	reset matrix to zero and resize
	const size_t numIndex = dd->num_indices();
	A.resize(0,0);
	A.resize(numIndex, numIndex);
	A.set(0.0);

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			AssembleStiffnessMatrix<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, A, u, m_pBoolMarker);
			break;
		case 2:
			AssembleStiffnessMatrix<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, A, u, m_pBoolMarker);
			AssembleStiffnessMatrix<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, A, u, m_pBoolMarker);
			break;
		case 3:
			AssembleStiffnessMatrix<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, A, u, m_pBoolMarker);
			AssembleStiffnessMatrix<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, A, u, m_pBoolMarker);
			AssembleStiffnessMatrix<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, A, u, m_pBoolMarker);
			AssembleStiffnessMatrix<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, A, u, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::assemble_stiffness_matrix:"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::assemble_stiffness_matrix:"
					" Assembling of elements of Dimension " << dim << " in "
					" subset "<<si<< " failed.");
	}

//	post process
	try{
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_jacobian(A, u, dd->grid_level());
	}
	}UG_CATCH_THROW("DomainDiscretization::assemble_stiffness_matrix:"
					" Cannot execute post process.");

//	Remember parallel storage type
#ifdef UG_PARALLEL
	A.set_storage_type(PST_ADDITIVE);
	TDD* pDD = const_cast<TDD*>(dd.get());
	CopyLayoutsAndCommunicatorIntoMatrix(A, *pDD);
#endif
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//  Time Independent (stationary)
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Jacobian (stationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
assemble_jacobian(matrix_type& J,
                  const vector_type& u,
                  ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	reset matrix to zero and resize
	const size_t numIndex = dd->num_indices();
	J.resize(0,0);
	J.resize(numIndex, numIndex);
	J.set(0.0);

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			AssembleJacobian<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, u, m_pBoolMarker);
			break;
		case 2:
			AssembleJacobian<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, u, m_pBoolMarker);
			AssembleJacobian<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, u, m_pBoolMarker);
			break;
		case 3:
			AssembleJacobian<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, u, m_pBoolMarker);
			AssembleJacobian<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, u, m_pBoolMarker);
			AssembleJacobian<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, u, m_pBoolMarker);
			AssembleJacobian<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, u, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::assemble_jacobian (stationary):"
							"Dimension "<<dim<<"(subset="<<si<<") not supported");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::assemble_jacobian (stationary):"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}

//	post process
	try{
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_jacobian(J, u, dd->grid_level());
	}
	}UG_CATCH_THROW("DomainDiscretization::assemble_jacobian:"
					" Cannot execute post process.");

//	Remember parallel storage type
#ifdef UG_PARALLEL
	J.set_storage_type(PST_ADDITIVE);
	TDD* pDD = const_cast<TDD*>(dd.get());
	CopyLayoutsAndCommunicatorIntoMatrix(J, *pDD);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// Defect (stationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
assemble_defect(vector_type& d,
                const vector_type& u,
                ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	reset matrix to zero and resize
	const size_t numIndex = dd->num_indices();
	d.resize(numIndex);
	d.set(0.0);

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			AssembleDefect<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, u, m_pBoolMarker);
			break;
		case 2:
			AssembleDefect<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, u, m_pBoolMarker);
			AssembleDefect<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, u, m_pBoolMarker);
			break;
		case 3:
			AssembleDefect<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, u, m_pBoolMarker);
			AssembleDefect<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, u, m_pBoolMarker);
			AssembleDefect<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, u, m_pBoolMarker);
			AssembleDefect<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, u, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::assemble_defect (stationary):"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::assemble_defect (stationary):"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}

//	post process
	try{
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_defect(d, u, dd->grid_level());
	}
	} UG_CATCH_THROW("Cannot adjust defect.");

//	Remember parallel storage type
#ifdef UG_PARALLEL
	d.set_storage_type(PST_ADDITIVE);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Matrix and RHS (stationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
assemble_linear(matrix_type& mat, vector_type& rhs,
                ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	reset matrix to zero and resize
	const size_t numIndex = dd->num_indices();
	mat.resize(0,0);
	mat.resize(numIndex, numIndex);
	mat.set(0.0);

	rhs.resize(numIndex);
	rhs.set(0.0);

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("DomainDiscretization: Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			AssembleLinear<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, m_pBoolMarker);
			break;
		case 2:
			AssembleLinear<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, m_pBoolMarker);
			AssembleLinear<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, m_pBoolMarker);
			break;
		case 3:
			AssembleLinear<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, m_pBoolMarker);
			AssembleLinear<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, m_pBoolMarker);
			AssembleLinear<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, m_pBoolMarker);
			AssembleLinear<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::assemble_linear (stationary):"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::assemble_linear (stationary):"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}

//	post process
	try{
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_linear(mat, rhs, dd->grid_level());
	}
	}UG_CATCH_THROW("DomainDiscretization::assemble_linear: Cannot post process.");

//	Remember parallel storage type
#ifdef UG_PARALLEL
	mat.set_storage_type(PST_ADDITIVE);
	TDD* pDD = const_cast<TDD*>(dd.get());
	CopyLayoutsAndCommunicatorIntoMatrix(mat, *pDD);
	rhs.set_storage_type(PST_ADDITIVE);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// RHS (stationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
assemble_rhs(vector_type& rhs,
			const vector_type& u,
			ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	reset matrix to zero and resize
	const size_t numIndex = dd->num_indices();
	rhs.resize(numIndex);
	rhs.set(0.0);

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			AssembleRhs<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, rhs, u, m_pBoolMarker);
			break;
		case 2:
			AssembleRhs<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, rhs, u, m_pBoolMarker);
			AssembleRhs<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, rhs, u, m_pBoolMarker);
			break;
		case 3:
			AssembleRhs<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, rhs, u, m_pBoolMarker);
			AssembleRhs<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, rhs, u, m_pBoolMarker);
			AssembleRhs<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, rhs, u, m_pBoolMarker);
			AssembleRhs<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, rhs, u, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::assemble_rhs (stationary):"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::assemble_rhs (stationary):"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}

//	post process
	try{
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_rhs(rhs, u, dd->grid_level());
	}
	}UG_CATCH_THROW("DomainDiscretization::assemble_rhs:"
					" Cannot execute post process.");

//	Remember parallel storage type
#ifdef UG_PARALLEL
	rhs.set_storage_type(PST_ADDITIVE);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// set constraints (stationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
adjust_solution(vector_type& u, ConstSmartPtr<TDD> dd)
{
	update_constraints();

	try{
//	constraints
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_solution(u, dd->grid_level());
	}

	} UG_CATCH_THROW("Cannot adjust solution.");
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//  Time Dependent (instationary)
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Prepare Timestep (instationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
prepare_timestep(ConstSmartPtr<VectorTimeSeries<vector_type> > vSol,
                ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			PrepareTimestep<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			break;
		case 2:
			PrepareTimestep<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			PrepareTimestep<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			break;
		case 3:
			PrepareTimestep<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			PrepareTimestep<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			PrepareTimestep<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			PrepareTimestep<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::prepare_timestep (instationary):"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::prepare_timestep (instationary):"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}

	//hier Dirichlet-Constraints abfragen?

}

///////////////////////////////////////////////////////////////////////////////
// Jacobian (instationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
assemble_jacobian(matrix_type& J,
                  ConstSmartPtr<VectorTimeSeries<vector_type> > vSol,
                  const number s_a0,
                  ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	reset matrix to zero and resize
	const size_t numIndex = dd->num_indices();
	J.resize(0,0);
	J.resize(numIndex, numIndex);
	J.set(0.0);

//	get current time
	const number time = vSol->time(0);

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			AssembleJacobian<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, vSol, s_a0, m_pBoolMarker);
			break;
		case 2:
			AssembleJacobian<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, vSol, s_a0, m_pBoolMarker);
			AssembleJacobian<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, vSol, s_a0, m_pBoolMarker);
			break;
		case 3:
			AssembleJacobian<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, vSol, s_a0, m_pBoolMarker);
			AssembleJacobian<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, vSol, s_a0, m_pBoolMarker);
			AssembleJacobian<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, vSol, s_a0, m_pBoolMarker);
			AssembleJacobian<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, J, vSol, s_a0, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::assemble_jacobian (instationary):"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::assemble_jacobian (instationary):"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}

//	post process
	try{
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_jacobian(J, *vSol->solution(0), dd->grid_level(), time);
	}
	}UG_CATCH_THROW("Cannot adjust jacobian.");

//	Remember parallel storage type
#ifdef UG_PARALLEL
	J.set_storage_type(PST_ADDITIVE);
	TDD* pDD = const_cast<TDD*>(dd.get());
	CopyLayoutsAndCommunicatorIntoMatrix(J, *pDD);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Defect (instationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
assemble_defect(vector_type& d,
                ConstSmartPtr<VectorTimeSeries<vector_type> > vSol,
                const std::vector<number>& vScaleMass,
                const std::vector<number>& vScaleStiff,
                ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	reset matrix to zero and resize
	const size_t numIndex = dd->num_indices();
	d.resize(numIndex);
	d.set(0.0);

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			AssembleDefect<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			break;
		case 2:
			AssembleDefect<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			AssembleDefect<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			break;
		case 3:
			AssembleDefect<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			AssembleDefect<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			AssembleDefect<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			AssembleDefect<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, d, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::assemble_defect (instationary):"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::assemble_defect (instationary):"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}

//	post process
	try{
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_defect(d, *vSol->solution(0), dd->grid_level(), vSol->time(0));
	}
	} UG_CATCH_THROW("Cannot adjust defect.");

//	Remember parallel storage type
#ifdef UG_PARALLEL
	d.set_storage_type(PST_ADDITIVE);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Matrix and RHS (instationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
assemble_linear(matrix_type& mat, vector_type& rhs,
                ConstSmartPtr<VectorTimeSeries<vector_type> > vSol,
                const std::vector<number>& vScaleMass,
                const std::vector<number>& vScaleStiff,
                ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			AssembleLinear<Edge,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			break;
		case 2:
			AssembleLinear<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			AssembleLinear<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			break;
		case 3:
			AssembleLinear<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			AssembleLinear<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			AssembleLinear<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			AssembleLinear<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, mat, rhs, vSol, vScaleMass, vScaleStiff, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::assemble_linear (instationary):"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::assemble_linear (instationary):"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}


//	post process
	try{
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_linear(mat, rhs, dd->grid_level(), vSol->time(0));
	}
	} UG_CATCH_THROW("Cannot adjust linear.");

//	Remember parallel storage type
#ifdef UG_PARALLEL
	mat.set_storage_type(PST_ADDITIVE);
	TDD* pDD = const_cast<TDD*>(dd.get());
	CopyLayoutsAndCommunicatorIntoMatrix(mat, *pDD);

	rhs.set_storage_type(PST_ADDITIVE);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// set constraint values (instationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
adjust_solution(vector_type& u, number time, ConstSmartPtr<TDD> dd)
{
	update_constraints();

	try{

//	constraints
	for(int type = 1; type < CT_ALL; type = type << 1){
		if(!(type & m_ConstraintTypesEnabled)) continue;
		for(size_t i = 0; i < m_vConstraint.size(); ++i)
			if(m_vConstraint[i]->type() & type)
				m_vConstraint[i]->adjust_solution(u, dd->grid_level(), time);
	}
	} UG_CATCH_THROW(" Cannot adjust solution.");
}

///////////////////////////////////////////////////////////////////////////////
// Finish Timestep (instationary)
///////////////////////////////////////////////////////////////////////////////
template <typename TDomain, typename TAlgebra>
template <typename TDD>
void DomainDiscretization<TDomain, TAlgebra>::
finish_timestep(ConstSmartPtr<VectorTimeSeries<vector_type> > vSol,
                ConstSmartPtr<TDD> dd)
{
//	update the elem discs
	update_disc_items();

//	Union of Subsets
	SubsetGroup unionSubsets;
	std::vector<SubsetGroup> vSSGrp;

//	create list of all subsets
	try{
		CreateSubsetGroups(vSSGrp, unionSubsets, m_vElemDisc, dd->subset_handler());
	}UG_CATCH_THROW("'DomainDiscretization': Can not create Subset Groups and Union.");

//	loop subsets
	for(size_t i = 0; i < unionSubsets.num_subsets(); ++i)
	{
	//	get subset
		const int si = unionSubsets[i];

	//	get dimension of the subset
		const int dim = DimensionOfSubset(*dd->subset_handler(), si);

	//	request if subset is regular grid
		bool bNonRegularGrid = !unionSubsets.regular_grid(i);

	//	overrule by regular grid if required
		if(m_bForceRegGrid) bNonRegularGrid = false;

	//	Elem Disc on the subset
		std::vector<IElemDisc*> vSubsetElemDisc;

	//	get all element discretizations that work on the subset
		GetElemDiscOnSubset(vSubsetElemDisc, m_vElemDisc, vSSGrp, si);

	//	assemble on suitable elements
		try
		{
		switch(dim)
		{
		case 1:
			FinishTimestep<Edge, TDD, TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			break;
		case 2:
			FinishTimestep<Triangle,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			FinishTimestep<Quadrilateral,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			break;
		case 3:
			FinishTimestep<Tetrahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			FinishTimestep<Pyramid,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			FinishTimestep<Prism,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			FinishTimestep<Hexahedron,TDD,TAlgebra>
				(vSubsetElemDisc, dd, si, bNonRegularGrid, vSol, m_pBoolMarker);
			break;
		default:
			UG_THROW("DomainDiscretization::finish_timestep (instationary):"
							"Dimension "<<dim<<" (subset="<<si<<") not supported.");
		}
		}
		UG_CATCH_THROW("DomainDiscretization::finish_timestep (instationary):"
						" Assembling of elements of Dimension " << dim << " in "
						" subset "<<si<< " failed.");
	}

}

} // end namespace ug

#endif /*__H__UG__LIB_DISC__SPATIAL_DISC__DOMAIN_DISC_IMPL__*/
