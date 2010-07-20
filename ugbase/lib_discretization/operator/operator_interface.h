/*
 * operator_interface.h
 *
 *  Created on: 22.02.2010
 *      Author: andreasvogel
 */

#ifndef __H__LIB_DISCRETIZATION__OPERATOR__OPERATOR_INTERFACE__
#define __H__LIB_DISCRETIZATION__OPERATOR__OPERATOR_INTERFACE__

namespace ug{

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Operator
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

// describes a mapping X->Y
template <typename X, typename Y>
class IOperator
{
	public:
		// domain space
		typedef X domain_function_type;

		// range space
		typedef Y codomain_function_type;

	public:
		// init
		virtual bool init() = 0;

		// prepare Operator
		virtual bool prepare(domain_function_type& u, codomain_function_type& d) = 0;

		// apply Operator, i.e. f := L(u);
		virtual bool apply(domain_function_type& u, codomain_function_type& d) = 0;

		// destructor
		virtual ~IOperator() {};
};

// describes a mapping X->Y
template <typename X, typename Y>
class IOperatorInverse
{
	public:
		// domain space
		typedef X domain_function_type;

		// range space
		typedef Y codomain_function_type;

	public:
		// init: This operator inverts the Operator N: Y -> X
		virtual bool init(IOperator<Y,X>& N) = 0;

		// prepare Operator
		virtual bool prepare(codomain_function_type& u) = 0;

		// apply Operator, i.e. N^{-1}(0) = u
		virtual bool apply(codomain_function_type& u) = 0;

		// destructor
		virtual ~IOperatorInverse() {};
};


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Linearized Operator
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////


// describes a mapping X->Y
template <typename X, typename Y>
class ILinearizedOperator
{
	public:
		// domain space
		typedef X domain_function_type;

		// range space
		typedef Y codomain_function_type;

	public:
		// init
		virtual bool init() = 0;

		// prepare J(u) for application of J(u)*c = d
		virtual bool prepare(domain_function_type& u, domain_function_type& c, codomain_function_type& d) = 0;

		// apply Operator, i.e. d = J(u)*c
		virtual bool apply(domain_function_type& c, codomain_function_type& d) = 0;

		// apply Operator, i.e. f = f - L*u;
		virtual bool apply_sub(domain_function_type& u, codomain_function_type& f) = 0;

		// destructor
		virtual ~ILinearizedOperator() {};
};

/* This Operator type behaves different on application. It not only computes c = B(u)*d, but also changes d. */
/* It is used in iterative schemes. */
template <typename X, typename Y>
class ILinearizedIteratorOperator
{
	public:
		// domain space
		typedef X domain_function_type;

		// range space
		typedef Y codomain_function_type;

	public:
		// prepare for Linearized Operator J(u)
		virtual bool init(ILinearizedOperator<Y, X>& J) = 0;

		// prepare B(u) for application of B(u)*d = c
		virtual bool prepare(domain_function_type& u, domain_function_type& d, codomain_function_type& c) = 0;

		/** apply
		 *
		 * This function computes a new correction c = B(u)*d by applying the Operator.
		 * The defect is updated, if updateDefect is true
		 *
		 * \param[in] 	d 				Defect
		 * \param[out]	c				Correction, c = B(u)*d
		 * \param[in]	updateDefect	if true, the defect is updated, d:= d - J(u)*c
		 */
		virtual bool apply(domain_function_type& d, codomain_function_type& c, bool updateDefect) = 0;

		// clone
		virtual ILinearizedIteratorOperator<X,Y>* clone() = 0;

		// destructor
		virtual ~ILinearizedIteratorOperator() {};
};

template <typename X, typename Y>
class ILinearizedOperatorInverse
{
	public:
		// domain space
		typedef X domain_function_type;

		// range space
		typedef Y codomain_function_type;

	public:
		// init for Linearized Operator A
		virtual bool init(ILinearizedOperator<Y,X>& A) = 0;

		// prepare Operator
		virtual bool prepare(codomain_function_type& u, domain_function_type& d_nl, codomain_function_type& c_nl) = 0;

		// Solve J(u)*c_nl = d_nl, such that c_nl = J(u)^{-1} d_nl
		// This is done by iterating: c_nl := c_nl + B(u)(d_nl - J(u)*c_nl)
		// In d_nl the last defect d := d_nl - J(u)*c_nl is returned
		// In the following:
		// c_nl, d_nl refer to the non-linear defect and correction as e.g. in J(u) * c_nl = d_nl as it appears in Newton scheme
		// c, d are the correction and defect for solving that linear equation iteratively.
		virtual bool apply(domain_function_type& d_nl, codomain_function_type& c_nl) = 0;

		// destructor
		virtual ~ILinearizedOperatorInverse() {};
};

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Linear Operator
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

template <typename X, typename Y>
class ILinearOperator : public ILinearizedOperator<X,Y>
{
	public:
		// domain space
		typedef X domain_function_type;

		// range space
		typedef Y codomain_function_type;

	public:
		// init
		virtual bool init() = 0;

		// prepare Operator
		virtual bool prepare(domain_function_type& u, codomain_function_type& f) = 0;

		// implement the interface for Linearized Operator
		virtual bool prepare(domain_function_type& u, domain_function_type& c, codomain_function_type& d) {return prepare(c,d);};

		// apply Operator, i.e. f = L*u; (or d = J*c in case of Linearized Operator, i.e. u = c, f = d)
		virtual bool apply(domain_function_type& u, codomain_function_type& f) = 0;

		// apply Operator, i.e. f = f - L*u;
		virtual bool apply_sub(domain_function_type& u, codomain_function_type& f) = 0;

		// destructor
		virtual ~ILinearOperator() {};
};

/* This Operator type behaves different on application. It not only computes c = B*d, but also changes d. */
/* It is used in iterative schemes. */
template <typename X, typename Y>
class ILinearIteratorOperator : public ILinearizedIteratorOperator<X,Y>
{
	public:
		// domain space
		typedef X domain_function_type;

		// range space
		typedef Y codomain_function_type;

	public:
		// prepare for Operator
		virtual bool init(ILinearizedOperator<Y,X>& A) = 0;

		// prepare for correction and defect
		virtual bool prepare(domain_function_type& d, codomain_function_type& c) = 0;

		// Implement Interface for Linearized Operator
		virtual bool prepare(domain_function_type& u, domain_function_type& d, codomain_function_type& c){return prepare(d,c);}

		/** apply
		 *
		 * This function computes a new correction c = B*d by applying the Operator.
		 * The defect is updated, if updateDefect is true
		 *
		 * \param[in] 	d 				Defect
		 * \param[out]	c				Correction, c = B*d
		 * \param[in]	updateDefect	if true, the defect is updated, d:= d - A*c
		 */
		virtual bool apply(domain_function_type& d, codomain_function_type& c, bool updateDefect) = 0;

		// destructor
		virtual ~ILinearIteratorOperator() {};
};

template <typename X, typename Y>
class ILinearOperatorInverse : public ILinearizedOperatorInverse<X,Y>
{
	public:
		// domain space
		typedef X domain_function_type;

		// range space
		typedef Y codomain_function_type;

	public:
		// init for Linearized Operator A
		virtual bool init(ILinearOperator<Y,X>& A) = 0;

		// prepare Operator
		virtual bool prepare(codomain_function_type& f, domain_function_type& u) = 0;

		// prepare Operator
		virtual bool prepare(codomain_function_type& u, domain_function_type& d, codomain_function_type& c) {return prepare(d,c);}

		// Solve A*u = b, such that u = A^{-1} b
		// This is done by iterating: u := u + B(b - A*u)
		// In b the last defect b := b - A*u is returned
		virtual bool apply(domain_function_type& f, codomain_function_type& u) = 0;

		// destructor
		virtual ~ILinearOperatorInverse() {};
};


} // end namespace ug

#endif
