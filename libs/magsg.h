/**
 * magnetic space group library
 * @author Tobias Weber
 * @date 7-apr-18
 * @license see 'LICENSE.EUPL' file
 */

#ifndef __MAG_SG_H__
#define __MAG_SG_H__

#include <string>
#include <vector>

#include "math_concepts.h"


template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class Spacegroups;



/**
 * a magnetic space group
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class Spacegroup
{
	friend class Spacegroups<t_mat, t_vec>;

private:
	std::string m_nameBNS, m_nameOG;
	std::string m_nrBNS, m_nrOG;

	// ------------------------------------------------------------------------
	// symmetry operations
	// rotation
	std::vector<t_mat> m_rotBNS, m_rotOG;

	// translation
	std::vector<t_vec> m_transBNS, m_transOG;

	// time inversion
	std::vector<typename t_mat::value_type> m_invBNS, m_invOG;
	// ------------------------------------------------------------------------


	// lattice definition
	std::vector<t_vec> m_latticeBNS, m_latticeOG;



public:
	Spacegroup() = default;
	~Spacegroup() = default;
};



/**
 * a collection of magnetic space groups
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class Spacegroups
{
private:
	std::vector<Spacegroup<t_mat, t_vec>> m_sgs;

public:
	bool Load(const std::string& strFile);

	Spacegroups() = default;
	~Spacegroups() = default;
};


#endif
