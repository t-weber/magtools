/**
 * magnetic space group library
 * @author Tobias Weber
 * @date 7-apr-18
 * @license see 'LICENSE.EUPL' file
 */

#include "magsg.h"

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace ptree = boost::property_tree;



bool Spacegroups::Load(const std::string& strFile)
{
	// load xml database
	ptree::ptree prop;
	try
	{
		ptree::read_xml(strFile, prop);
	}
	catch(const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
		return false;
	}


	// iterate space groups
	const auto& groups = prop.get_child_optional("mag_groups");
	if(!groups)
	{
		std::cerr << "No space groups defined." << std::endl;
		return false;
	}

	for(const auto& group : *groups)
	{
		auto nameBNS = group.second.get_optional<std::string>("bns.name");
		auto nrBNS = group.second.get_optional<std::string>("bns.nr");
		const auto& lattBNS = group.second.get_child_optional("bns.latt");
		const auto& opsBNS = group.second.get_child_optional("bns.ops");
		const auto& wycBNS = group.second.get_child_optional("bns.wyc");

		auto nameOG = group.second.get_optional<std::string>("og.name");
		auto nrOG = group.second.get_optional<std::string>("og.nr");
		const auto& lattOG = group.second.get_child_optional("og.latt");
		const auto& opsOG = group.second.get_child_optional("og.ops");
		const auto& wycOG = group.second.get_child_optional("og.wyc");

		const auto& bns2og = group.second.get_child_optional("bns2og");


		Spacegroup sg;
		sg.m_nameBNS = *nameBNS;
		sg.m_nameOG = *nameOG;
		sg.m_nrBNS = *nrBNS;
		sg.m_nrOG = *nrOG;

		m_sgs.emplace_back(std::move(sg));
	}


	return true;
}



/**
 * test: g++ -std=c++14 -o magsg magsg.cpp -lboost_iostreams
 */
int main()
{
	Spacegroups sgs;
	sgs.Load("magsg.xml");

	return 0;
}
