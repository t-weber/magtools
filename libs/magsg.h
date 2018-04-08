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


// ----------------------------------------------------------------------------
/**
 * forward declarations
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class Spacegroups;
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * Symmetry operations
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class Symmetry
{
	friend class Spacegroups<t_mat, t_vec>;

private:
	// rotations
	std::vector<t_mat> m_rot;

	// translations
	std::vector<t_vec> m_trans;

	// time inversions
	std::vector<typename t_mat::value_type> m_inv;

public:
	Symmetry() = default;
	~Symmetry() = default;
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * Wyckoff positions
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class WycPositions
{
	friend class Spacegroups<t_mat, t_vec>;

private:
	std::string m_name;

	// multiplicity
	typename t_mat::value_type m_mult;

	// structural & magnetic rotations
	std::vector<t_mat> m_rot, m_rotMag;

	// translations
	std::vector<t_vec> m_trans;

public:
	WycPositions() = default;
	~WycPositions() = default;
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
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

	// lattice definition
	std::vector<t_vec> m_latticeBNS, m_latticeOG;

	// symmetry operations
	Symmetry<t_mat, t_vec> m_symBNS, m_symOG;

	// Wyckoff positions
	std::vector<WycPositions<t_mat, t_vec>> m_wycBNS, m_wycOG;

public:
	Spacegroup() = default;
	~Spacegroup() = default;
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
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
// ----------------------------------------------------------------------------


#endif
