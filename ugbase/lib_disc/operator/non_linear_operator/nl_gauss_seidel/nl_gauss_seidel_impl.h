/*
 * nl_gauss_seidel_impl.h
 *
 *  Created on: 07.01.2013
 *  (main parts are based on the structure of
 *  	newton_impl.h by Andreas Vogel)
 *
 *  Author: raphaelprohl
 *
 *  Nonlinear GaussSeidel-method: (c.f. "Iterative Solution of nonlinear
 *  				equations in several variables" by Ortega/Rheinboldt)
 *
 * 	Let L(u) denote a nonlinear functional of n components (l_1,...,l_n).
 * 	Then the basic step of the nonlinear GaussSeidel method is to solve the
 * 	i-th equation
 *
 * 	l_i(u_1^{k+1},...,u_{i-1}^{k+1},u_i,u_{i+1}^{k},...,u_{n}^{k}) = 0
 *
 * 	for u_i and to set u_i^{k+1} = u_i. Here k denotes the iteration-index.
 * 	Note, that the already computed, updated values (.)^{k+1} are used in this
 * 	method.
 * 	Thus, in order to obtain u^{k+1} from u^k, we solve successively the n
 * 	dimensional nonlinear equations for i = 1,...,n. Here this is done
 * 	by a scalar newton step for every i. But every other scalar nonlinear method
 * 	could be applied as well.
 *
 * 	Using a damped version of the nonlinear GaussSeidel method (= nonlinear
 * 	SOR-method) results in the following update of the variables
 *
 * 	u_i^{k+1} = u_i^k + damp * (u_i -u_i^k).
 */

#ifndef NL_GAUSS_SEIDEL_IMPL_H_
#define NL_GAUSS_SEIDEL_IMPL_H_

#include "lib_disc/function_spaces/grid_function_util.h"
#include "nl_gauss_seidel.h"

namespace ug{

template <typename TDomain, typename TAlgebra>
NLGaussSeidelSolver<TDomain,TAlgebra>::
NLGaussSeidelSolver(SmartPtr<approx_space_type> spApproxSpace,
			SmartPtr<IConvergenceCheck<vector_type> > spConvCheck) :
			m_spApproxSpace(spApproxSpace),
			m_spConvCheck(spConvCheck),
			m_damp(1.0)
{};

template <typename TDomain, typename TAlgebra>
NLGaussSeidelSolver<TDomain,TAlgebra>::
NLGaussSeidelSolver() :
	m_spApproxSpace(NULL),
	m_spConvCheck(new StdConvCheck<vector_type>(10, 1e-8, 1e-10, true)),
	m_damp(1.0)
{};

template <typename TDomain, typename TAlgebra>
NLGaussSeidelSolver<TDomain,TAlgebra>::
NLGaussSeidelSolver(SmartPtr<IOperator<vector_type> > N) :
	m_spApproxSpace(NULL),
	m_spConvCheck(new StdConvCheck<vector_type>(10, 1e-8, 1e-10, true)),
	m_damp(1.0)
{
	init(N);
};

template <typename TDomain, typename TAlgebra>
NLGaussSeidelSolver<TDomain,TAlgebra>::
NLGaussSeidelSolver(IAssemble<algebra_type>* pAss) :
	m_spApproxSpace(NULL),
	m_spConvCheck(new StdConvCheck<vector_type>(10, 1e-8, 1e-10, true)),
	m_damp(1.0)
{
	m_pAss = pAss;
	m_N = SmartPtr<AssembledOperator<TAlgebra> >(new AssembledOperator<TAlgebra>(*m_pAss));
};

template <typename TDomain, typename TAlgebra>
void NLGaussSeidelSolver<TDomain, TAlgebra>::
set_convergence_check(SmartPtr<IConvergenceCheck<vector_type> > spConvCheck)
{
	m_spConvCheck = spConvCheck;
	m_spConvCheck->set_offset(3);
	m_spConvCheck->set_symbol('#');
	m_spConvCheck->set_name("Nonlinear Gauss Seidel Solver");
}

template <typename TDomain, typename TAlgebra>
bool
NLGaussSeidelSolver<TDomain,TAlgebra>::
init(SmartPtr<IOperator<vector_type> > N)
{
	m_N = N.template cast_dynamic<AssembledOperator<TAlgebra> >();
	if(m_N.invalid())
		UG_THROW("NLGaussSeidelSolver: currently only works for AssembledDiscreteOperator.");

	m_pAss = m_N->get_assemble();
	return true;
}

template <typename TDomain, typename TAlgebra>
bool NLGaussSeidelSolver<TDomain,TAlgebra>::prepare(vector_type& u)
{
//	todo: maybe remove this from interface

	//	hier die Nachbarschaft der DoFs bestimmen?!
	//	d.h. jedem DoF sein geometrisches Objekt zuweisen?! (�ber Array)
	//	muss hier eventuell approxSpace rein?
	return true;
}

template <typename TDomain, typename TAlgebra>
bool NLGaussSeidelSolver<TDomain,TAlgebra>::apply(vector_type& u)
{
	/*	TODO: momentan wird zum L�sen der komponentenweisen
	 * 	nichtlinearen Gleichungen ein skalares Newton-Verfahren benutzt!
	 * 	Der nichtlineare Gauss-Seidel kann allerdings auch mit anderen Verfahren
	 * 	(Sekantenverfahren, ect...) zur L�sung dieser Teilprobleme
	 * 	aufgesetzt werden.-> Verallgemeinerung �ber set_nonlinear_solver? */

	//	Check for approxSpace
	if(m_spApproxSpace.invalid())
		UG_THROW("NLGaussSeidelSolver::apply: Approximation Space not set.");

	//	Jacobian
	if(m_J.invalid() || m_J->discretization() != m_pAss) {
		m_J = CreateSmartPtr(new AssembledLinearOperator<TAlgebra>(*m_pAss));
		m_J->set_level(m_N->level());
	}

	//	resize
	try{
		m_d.resize(u.size()); m_d = u;
		m_c.resize(u.size()); m_c = u;
	}UG_CATCH_THROW("NLGaussSeidelSolver::apply: Resize of Defect/Correction failed.");

	//	Set dirichlet values
	try{
		m_N->prepare(m_d, u);
	}
	UG_CATCH_THROW("NLGaussSeidelSolver::apply: Prepare of Operator failed.");

	// 	Compute first Defect d = L(u)
	try{
		m_N->apply(m_d, u);
	}UG_CATCH_THROW("NLGaussSeidelSolver::apply: "
			"Computation of Start-Defect failed.");

	// 	start convergence check
	m_spConvCheck->start(m_d);

	//	get #indices of gridFunction u
	size_t n_indices = u.size();
	matrix_type J, J_inv;

	//	loop iteration
	while(!m_spConvCheck->iteration_ended())
	{
		// 	set correction c = 0
		if(!m_c.set(0.0))
		{
			UG_LOG("ERROR in 'NLGaussSeidelSolver::apply':"
					" Cannot reset correction to zero.\n");
			return false;
		}

		//	loop all DoFs
		for (size_t i = 0; i < n_indices; i++)
		{
			// 	Compute Jacobian J(u) using the updated u-components
			try{
				m_J->init(u);
			}UG_CATCH_THROW("NLGaussSeidelSolver::apply: "
					"Initialization of Jacobian failed.");

			J = m_J->get_matrix();
			//	copy layouts from parallel matrix J
			J_inv = J;

			//	get i,i-th block of J: J(i,i)
			//	depending on the AlgebraType J(i,i) is a 1x1, 2x2, 3x3 Matrix
			//	invert this block
			//	TODO: replace this inversion!
			GetInverse(J_inv(i,i), J(i,i));

			//	m_c_i = m_damp * d_i /J_ii
			MatMult(m_c[i], m_damp, J_inv(i,i), m_d[i]);

			// 	update i-th block of solution
			u[i] -= m_c[i];

			// 	Compute d = L(u) using the updated u-blocks
			m_N->prepare(m_d, u);
			m_N->apply(m_d, u);

			// 	TODO: anstatt die ganzen Eintr�ge zu durchlaufen,
			//	nur die Nachbarschaft einer Komponente in der n�chsten Iteration abgrasen!
		}

		// 	check convergence
		m_spConvCheck->update(m_d);
	}

	return m_spConvCheck->post();
}

}

#endif /* NL_GAUSS_SEIDEL_IMPL_H_ */
