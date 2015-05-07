/*
 * domain_discretization_bridge.cpp
 *
 *  Created on: 22.09.2010
 *      Author: andreasvogel
 */

// extern headers
#include <iostream>
#include <sstream>
#include <string>

// include bridge
#include "bridge/bridge.h"
#include "bridge/util.h"
#include "bridge/util_domain_algebra_dependent.h"

// lib_disc includes
#include "lib_disc/domain.h"
#include "lib_disc/spatial_disc/domain_disc.h"
#include "lib_disc/parallelization/domain_distribution.h"
#include "lib_disc/function_spaces/grid_function.h"


using namespace std;

namespace ug{
namespace bridge{
namespace DomainDisc{

/**
 * \defgroup domaindisc_bridge Domain Discretization Bridge
 * \ingroup disc_bridge
 * \{
 */

/**
 * Class exporting the functionality. All functionality that is to
 * be used in scripts or visualization must be registered here.
 */
struct Functionality
{

/**
 * Function called for the registration of Domain and Algebra dependent parts.
 * All Functions and Classes depending on both Domain and Algebra
 * are to be placed here when registering. The method is called for all
 * available Domain and Algebra types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <typename TDomain, typename TAlgebra>
static void DomainAlgebra(Registry& reg, string grp)
{
	string suffix = GetDomainAlgebraSuffix<TDomain,TAlgebra>();
	string tag = GetDomainAlgebraTag<TDomain,TAlgebra>();

//	group string
	string approxGrp = grp; approxGrp.append("/ApproximationSpace");
	string domDiscGrp = grp; domDiscGrp.append("/SpatialDisc");

//	DomainDiscretization
	{
		typedef IDomainDiscretization<TAlgebra> TBase;
		typedef DomainDiscretization<TDomain, TAlgebra> T;
		string name = string("DomainDiscretization").append(suffix);
		reg.add_class_<T, TBase>(name, domDiscGrp)
			.template add_constructor<void (*)(SmartPtr<ApproximationSpace<TDomain> >)>("ApproximationSpace")
			.add_method("add", static_cast<void (T::*)(SmartPtr<IDomainConstraint<TDomain, TAlgebra> >)>(&T::add), "", "Post Process")
			.add_method("remove", static_cast<void (T::*)(SmartPtr<IDomainConstraint<TDomain, TAlgebra> >)>(&T::remove), "", "Post Process")
			.add_method("add", static_cast<void (T::*)(SmartPtr<IElemDisc<TDomain> >)>(&T::add), "", "Element Discretization")
			.add_method("add", static_cast<void (T::*)(SmartPtr<IDiscretizationItem<TDomain, TAlgebra> >)>(&T::add), "", "DiscItem")
			.add_method("assemble_linear", static_cast<void (T::*)(typename TAlgebra::matrix_type&, GridFunction<TDomain, TAlgebra>&)>(&T::assemble_linear))
			.add_method("assemble_rhs", static_cast<void (T::*)(typename TAlgebra::vector_type&, GridFunction<TDomain, TAlgebra>&)>(&T::assemble_rhs))
			.add_method("assemble_rhs", static_cast<void (T::*)(GridFunction<TDomain, TAlgebra>&)>(&T::assemble_rhs))
			.add_method("adjust_solution", static_cast<void (T::*)(GridFunction<TDomain, TAlgebra>&)>(&T::adjust_solution))
			.add_method("calc_error", static_cast<void (T::*)(const GridFunction<TDomain, TAlgebra>&)>(&T::calc_error), "", "Calculate element-wise error indicators from error estimator")
			.add_method("calc_error", static_cast<void (T::*)(const GridFunction<TDomain, TAlgebra>&, typename TAlgebra::vector_type*)>(&T::calc_error), "", "Calculate element-wise error indicators from error estimator")
			.add_method("mark_for_refinement", static_cast<void (T::*)(IRefiner&, number, number, int)>(&T::mark_for_refinement), "",
				"mark elements for refinement according to calculated error indicators")
			.add_method("mark_for_coarsening", static_cast<void (T::*)(IRefiner&, number, number, int)>(&T::mark_for_coarsening), "",
				"mark elements for coarsening according to calculated error indicators")
			.add_method("mark_with_strategy", &T::mark_with_strategy)
			.add_method("invalidate_error", &T::invalidate_error, "", "Marks error indicators as invalid, "
				"which will prohibit refining and coarsening before a new call to calc_error.")
			.add_method("is_error_valid", &T::is_error_valid, "", "Returns whether error values are valid")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "DomainDiscretization", tag);
	}

//	IDiscretizationItem
	{
		typedef IDiscretizationItem<TDomain, TAlgebra> T;
		string name = string("IDiscretizationItem").append(suffix);
		reg.add_class_<T>(name, domDiscGrp);
		reg.add_class_to_group(name, "IDiscretizationItem", tag);
	}
}

/**
 * Function called for the registration of Domain dependent parts.
 * All Functions and Classes depending on the Domain
 * are to be placed here when registering. The method is called for all
 * available Domain types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <typename TDomain>
static void Domain(Registry& reg, string grp)
{
	string suffix = GetDomainSuffix<TDomain>();
	string tag = GetDomainTag<TDomain>();
	
//	group string
	string domDiscGrp = grp; domDiscGrp.append("/SpatialDisc");
	

//	IErrEstData
	{
		typedef IErrEstData<TDomain> T;
		string name = string("IErrEstData").append(suffix);
		reg.add_class_<T>(name, domDiscGrp)
			.add_method("set_consider_me", &T::set_consider_me, "", "", "", "")
			.add_method("set_scaling_factor", &T::set_scaling_factor, "", "", "", "");
		reg.add_class_to_group(name, "ErrEstData", tag);
	}

//	SideFluxErrEstData
	{
		typedef SideFluxErrEstData<TDomain> T;
		typedef IErrEstData<TDomain> TBase;
		string name = string("SideFluxErrEstData").append(suffix);
		reg.add_class_<T, TBase>(name, domDiscGrp)
			.template add_constructor<void (*) ()>()
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "SideFluxErrEstData", tag);
	}

//	SideAndElemErrEstData
	{
		typedef SideAndElemErrEstData<TDomain> T;
		typedef IErrEstData<TDomain> TBase;
		string name = string("SideAndElemErrEstData").append(suffix);
		reg.add_class_<T, TBase>(name, domDiscGrp)
			.template add_constructor<void (*) (std::size_t, std::size_t)>
				("integration order for sides#integration order for elements", "", "", "")
			.template add_constructor<void (*) (std::size_t, std::size_t, const char*)>
				("integration order for sides#integration order for elements#subsets considered", "", "", "")
			.template add_constructor<void (*) (std::size_t, std::size_t, std::vector<std::string>)>
				("integration order for sides#integration order for elements#subsets considered", "", "", "")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "SideAndElemErrEstData", tag);
	}

//	MultipleSideAndElemErrEstData
	{
		typedef MultipleSideAndElemErrEstData<TDomain> T;
		typedef IErrEstData<TDomain> TBase;
		string name = string("MultipleSideAndElemErrEstData").append(suffix);
		reg.add_class_<T, TBase>(name, domDiscGrp)
			.template add_constructor<void (*) (ConstSmartPtr<ApproximationSpace<TDomain> >)> ()
			.add_method("add", static_cast<void (T::*)(SmartPtr<SideAndElemErrEstData<TDomain> >, const char* )>(&T::add),
						"ErrEstData object", "", "Add existing ErrEstData objects one at a time. "
						"+++ TAKE CARE: The order matters! +++ "
						"ElemDiscs given this object can access the underlying error estimator "
						"data objects in the order of addition.", "")
			.set_construct_as_smart_pointer(true);
		reg.add_class_to_group(name, "MultipleSideAndElemErrEstData", tag);
	}
}

/**
 * Function called for the registration of Dimension dependent parts.
 * All Functions and Classes depending on the Dimension
 * are to be placed here when registering. The method is called for all
 * available Dimension types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <int dim>
static void Dimension(Registry& reg, string grp)
{
	string suffix = GetDimensionSuffix<dim>();
	string tag = GetDimensionTag<dim>();

}

/**
 * Function called for the registration of Algebra dependent parts.
 * All Functions and Classes depending on Algebra
 * are to be placed here when registering. The method is called for all
 * available Algebra types, based on the current build options.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
template <typename TAlgebra>
static void Algebra(Registry& reg, string grp)
{
	string suffix = GetAlgebraSuffix<TAlgebra>();
	string tag = GetAlgebraTag<TAlgebra>();

}

/**
 * Function called for the registration of Domain and Algebra independent parts.
 * All Functions and Classes not depending on Domain and Algebra
 * are to be placed here when registering.
 *
 * @param reg				registry
 * @param parentGroup		group for sorting of functionality
 */
static void Common(Registry& reg, string grp)
{
//	group string
	string domDiscGrp = grp; domDiscGrp.append("/SpatialDisc");
}

}; // end Functionality

// end group domaindisc_bridge
/// \}

}// namespace DomainDisc

/// \addtogroup domaindisc_bridge
void RegisterBridge_DomainDisc(Registry& reg, string grp)
{
	grp.append("/Discretization");
	typedef DomainDisc::Functionality Functionality;

	try{
//		RegisterCommon<Functionality>(reg,grp);
//		RegisterDimensionDependent<Functionality>(reg,grp);
		RegisterDomainDependent<Functionality>(reg,grp);
//		RegisterAlgebraDependent<Functionality>(reg,grp);
		RegisterDomainAlgebraDependent<Functionality>(reg,grp);
	}
	UG_REGISTRY_CATCH_THROW(grp);
}

}//	end of namespace bridge
}//	end of namespace ug
