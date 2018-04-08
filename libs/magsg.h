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
#include <memory>

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

	const std::vector<t_mat>& GetRotations() const { return m_rot; }
	const std::vector<t_vec>& GetTranslations() const { return m_trans; }
	const std::vector<typename t_mat::value_type>& GetInversions() const { return m_inv; }
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
	std::string m_letter;

	// multiplicity
	int m_mult = 0;

	// structural & magnetic rotations
	std::vector<t_mat> m_rot, m_rotMag;

	// translations
	std::vector<t_vec> m_trans;

public:
	WycPositions() = default;
	~WycPositions() = default;

	const std::string& GetLetter() const { return m_letter; }
	int GetMultiplicity() const { return m_mult; }
	std::string GetName() const { return std::to_string(m_mult) + m_letter; }

	const std::vector<t_mat>& GetRotations() const { return m_rot; }
	const std::vector<t_mat>& GetRotationsMag() const { return m_rotMag; }
	const std::vector<t_vec>& GetTranslations() const { return m_trans; }
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
	std::shared_ptr<std::vector<t_vec>> m_latticeBNS;
	std::shared_ptr<std::vector<t_vec>> m_latticeOG;

	// symmetry operations
	std::shared_ptr<Symmetry<t_mat, t_vec>> m_symBNS;
	std::shared_ptr<Symmetry<t_mat, t_vec>> m_symOG;

	// Wyckoff positions
	std::shared_ptr<std::vector<WycPositions<t_mat, t_vec>>> m_wycBNS;
	std::shared_ptr<std::vector<WycPositions<t_mat, t_vec>>> m_wycOG;

public:
	Spacegroup() = default;
	~Spacegroup() = default;

	const std::string& GetName(bool bBNS=1) const
	{ return bBNS ? m_nameBNS : m_nameOG; }

	const std::string& GetNumber(bool bBNS=1) const
	{ return bBNS ? m_nrBNS : m_nrOG; }

	const std::vector<t_vec>* GetLattice(bool bBNS=1) const
	{ return bBNS ? m_latticeBNS.get() : m_latticeOG.get(); }

	const std::vector<Symmetry<t_mat, t_vec>>* GetSymmetries(bool bBNS=1) const
	{ return bBNS ? m_symBNS.get() : m_symOG.get(); }

	const std::vector<WycPositions<t_mat, t_vec>>* GetWycPositions(bool bBNS=1) const
	{ return bBNS ? m_wycBNS.get() : m_wycOG.get(); }
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
	Spacegroups() = default;
	~Spacegroups() = default;

	bool Load(const std::string& strFile);

	const std::vector<Spacegroup<t_mat, t_vec>>* GetSpacegroups() const
	{ return &m_sgs; }
};
// ----------------------------------------------------------------------------


#endif
