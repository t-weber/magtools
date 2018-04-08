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


#include "math_algos.h"


template<class t_mat, class t_vec>
bool Spacegroups<t_mat, t_vec>::Load(const std::string& strFile)
{
	using t_real = typename t_mat::value_type;

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
		// --------------------------------------------------------------------
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
		// --------------------------------------------------------------------


		// --------------------------------------------------------------------
		Spacegroup<t_mat, t_vec> sg;

		sg.m_nameBNS = *nameBNS;
		sg.m_nameOG = *nameOG;
		sg.m_nrBNS = *nrBNS;
		sg.m_nrOG = *nrOG;
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// reads in a vector
		auto get_vec = [](const std::string& str) -> t_vec
		{
			t_vec vec = m::zero<t_vec>(3);

			std::istringstream istr(str);
			for(std::size_t i=0; i<vec.size(); ++i)
				istr >> vec[i];

			return vec;
		};

		// reads in a matrix
		auto get_mat = [](const std::string& str) -> t_mat
		{
			t_mat mat = m::zero<t_mat>(3,3);

			std::istringstream istr(str);
			for(std::size_t i=0; i<mat.size1(); ++i)
				for(std::size_t j=0; j<mat.size2(); ++j)
					istr >> mat(i,j);

			return mat;
		};
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// iterate symmetry trafos
		auto load_ops = [&get_vec, &get_mat](const decltype(opsBNS)& ops)
			-> std::tuple<std::vector<t_mat>, std::vector<t_vec>, std::vector<t_real>>
		{
			std::vector<t_mat> rotations;
			std::vector<t_vec> translations;
			std::vector<t_real> inversions;

			for(std::size_t iOp=1; true; ++iOp)
			{
				std::string strOp = std::to_string(iOp);
				std::string nameTrafo = "R" + strOp;
				std::string nameTrans = "v" + strOp;
				std::string nameDiv = "d" + strOp;
				std::string nameInv = "t" + strOp;

				auto opTrafo = ops->get_optional<std::string>(nameTrafo);
				auto opTrans = ops->get_optional<std::string>(nameTrans);
				auto opdiv = ops->get_optional<t_real>(nameDiv);
				auto opinv = ops->get_optional<t_real>(nameInv);

				if(!opTrafo)
					break;

				t_real div = opdiv ? *opdiv : t_real(1);
				t_real inv = opinv ? *opinv : t_real(1);
				t_mat rot = opTrafo ? get_mat(*opTrafo) : m::unit<t_mat>(3,3);
				t_vec trans = opTrans ? get_vec(*opTrans) : m::zero<t_vec>(3);
				trans /= div;

				rotations.emplace_back(std::move(rot));
				translations.emplace_back(std::move(trans));
				inversions.push_back(inv);
			}

			return std::make_tuple(std::move(rotations), std::move(translations), std::move(inversions));
		};

		if(opsBNS)
			std::tie(sg.m_symBNS.m_rot, sg.m_symBNS.m_trans, sg.m_symBNS.m_inv) = std::move(load_ops(opsBNS));
		if(opsOG)
			std::tie(sg.m_symOG.m_rot, sg.m_symOG.m_trans, sg.m_symOG.m_inv) = std::move(load_ops(opsOG));
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// iterate over lattice vectors
		auto load_latt = [&get_vec](const decltype(lattBNS)& latt)
			-> std::tuple<std::vector<t_vec>>
		{
			std::vector<t_vec> vectors;

			for(std::size_t iVec=1; true; ++iVec)
			{
				std::string strVec = std::to_string(iVec);
				std::string nameVec = "v" + strVec;
				std::string nameDiv = "d" + strVec;

				auto opVec = latt->get_optional<std::string>(nameVec);
				auto opdiv = latt->get_optional<t_real>(nameDiv);

				if(!opVec)
					break;

				t_real div = opdiv ? *opdiv : t_real(1);
				t_vec vec = opVec ? get_vec(*opVec) : m::zero<t_vec>(3);
				vec /= div;

				vectors.emplace_back(std::move(vec));
			}

			return std::make_tuple(std::move(vectors));
		};

		if(lattBNS)
			std::tie(sg.m_latticeBNS) = std::move(load_latt(lattBNS));
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// iterate wyckoff positions
		auto load_wyc = [&get_vec, &get_mat](const decltype(wycBNS)& wycs)
			-> std::vector<WycPositions<t_mat, t_vec>>
		{
			std::vector<WycPositions<t_mat, t_vec>> vecWyc;

			for(std::size_t iWyc=1; true; ++iWyc)
			{
				std::string nameSite = "site" + std::to_string(iWyc);
				auto wyc = wycs->get_child_optional(nameSite);
				if(!wyc) break;


				WycPositions<t_mat, t_vec> wycpos;

				for(std::size_t iPos=1; true; ++iPos)
				{
					std::string strPos = std::to_string(iPos);
					std::string nameRot = "R" + strPos;
					std::string nameRotMag = "M" + strPos;
					std::string nameTrans = "v" + strPos;
					std::string nameDiv = "d" + strPos;

					auto opRot = wyc->get_optional<std::string>(nameRot);
					auto opRotMag = wyc->get_optional<std::string>(nameRotMag);
					auto opTrans = wyc->get_optional<std::string>(nameTrans);
					auto opdiv = wyc->get_optional<t_real>(nameDiv);

					if(!opRot)
						break;

					t_real div = opdiv ? *opdiv : t_real(1);
					t_mat rot = opRot ? get_mat(*opRot) : m::unit<t_mat>(3,3);
					t_mat rotMag = opRotMag ? get_mat(*opRotMag) : rot;
					t_vec trans = opTrans ? get_vec(*opTrans) : m::zero<t_vec>(3);
					trans /= div;

					wycpos.m_rot.emplace_back(std::move(rot));
					wycpos.m_rotMag.emplace_back(std::move(rotMag));
					wycpos.m_trans.emplace_back(std::move(trans));
				}

				vecWyc.emplace_back(std::move(wycpos));
			}

			return vecWyc;
		};


		if(wycBNS) sg.m_wycBNS = std::move(load_wyc(wycBNS));
		if(wycOG) sg.m_wycOG = std::move(load_wyc(wycOG));
		// --------------------------------------------------------------------



		m_sgs.emplace_back(std::move(sg));
	}


	return true;
}




// ----------------------------------------------------------------------------
// test: g++ -std=c++17 -fconcepts -o magsg magsg.cpp -lboost_iostreams


#include "math_conts.h"
using namespace m_ops;

int main()
{
	using t_real = double;
	using t_vec = std::vector<t_real>;
	using t_mat = m::mat<t_real, std::vector>;

	Spacegroups<t_mat, t_vec> sgs;
	sgs.Load("magsg.xml");

	return 0;
}
// ----------------------------------------------------------------------------
