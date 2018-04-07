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


class Spacegroups;


class Spacegroup
{ friend class Spacegroups;
private:
	std::string m_nameBNS, m_nameOG;
	std::string m_nrBNS, m_nrOG;

public:
	Spacegroup() = default;
	~Spacegroup() = default;
};


class Spacegroups
{
private:
	std::vector<Spacegroup> m_sgs;

public:
	bool Load(const std::string& strFile);

	Spacegroups() = default;
	~Spacegroups() = default;
};


#endif

