/**
 * converts magnetic space group table
 * @author Tobias Weber
 * @date 18-nov-17
 */

#include <iostream>
#include <fstream>
#include <tuple>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
namespace ublas = boost::numeric::ublas;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace ptree = boost::property_tree;

#include <boost/algorithm/string/trim.hpp>
namespace algo = boost::algorithm;


using t_real = double;
using t_mat = ublas::matrix<t_real>;
using t_vec = ublas::vector<t_real>;


template<class T>
T get_num(std::istream& istr)
{
	T t;
	istr >> t;
	return t;
}


std::string get_string(std::istream& istr)
{
	// skip whitespace
	while(1)
	{
		char c = istr.get();
		if(c!=' ' && c!='\t')
		{
			istr.unget();
			break;
		}
	}

	// string start
	char c = istr.get();
	if(c != '\"')
	{
		istr.unget();
		std::cerr << "Expected a string." << std::endl;
	}

	// read string
	std::string str;
	while(1)
	{
		c = istr.get();
		// end of string?
		if(c == '\"')
			break;

		str += c;
	}

	algo::trim(str);
	return str;
}


t_mat get_matrix(std::size_t N, std::size_t M, std::istream& istr)
{
	t_mat mat(N,M);

	for(std::size_t i=0; i<N; ++i)
		for(std::size_t j=0; j<M; ++j)
			istr >> mat(i,j);

	return mat;
}


t_vec get_vector(std::size_t N, std::istream& istr)
{
	t_vec vec(N);

	for(std::size_t i=0; i<N; ++i)
		istr >> vec(i);

	return vec;
}


std::tuple<std::string, t_mat> get_pointgroup_op(std::istream& istr)
{
	int iNum = get_num<int>(istr);

	std::string strName = get_string(istr);
	std::string strOpXYZ = get_string(istr);
	t_mat matOp = get_matrix(3,3, istr);

	//std::cout << strName << ": " << matOp << "\n";
	return { strName, matOp };
}


void convert_spacegroup(std::istream& istr, ptree::ptree& prop, const std::string& strPath,
	const std::vector<std::tuple<std::string, t_mat>>* pPtOps = nullptr,
	const std::vector<std::tuple<std::string, t_mat>>* pHexPtOps = nullptr)
{
	int iNrBNS[2];
	istr >> iNrBNS[0] >> iNrBNS[1];
	std::string strNrBNS = get_string(istr);
	std::string strSGBNS = get_string(istr);

	int iNrOG[3];
	istr >> iNrOG[0] >> iNrOG[1] >> iNrOG[2];
	std::string strNrOG = get_string(istr);
	std::string strSGOG = get_string(istr);

	prop.put(strPath + "bns.name", strSGBNS);
	prop.put(strPath + "og.name", strSGOG);


	int iTy = get_num<int>(istr);
	if(iTy == 4)	// BNS -> OG trafo
	{
		t_mat matBNS2OG = get_matrix(3,3, istr);
		t_vec vecBNS2OG = get_vector(3, istr);
		t_real numBNS2OG = get_num<t_real>(istr);

		prop.put(strPath + "bns2og.mat", matBNS2OG);
		prop.put(strPath + "bns2og.vec", vecBNS2OG);
		prop.put(strPath + "bns2og.num", numBNS2OG);
	}


	std::size_t iNumOpsBNS = get_num<std::size_t>(istr);
	for(std::size_t iPtOp=0; iPtOp<iNumOpsBNS; ++iPtOp)
	{
		std::size_t iOperBNS = get_num<std::size_t>(istr);
		t_vec vecBNS = get_vector(3, istr);
		t_real numBNS = get_num<t_real>(istr);
		int itInvBNS = get_num<int>(istr);

		// TODO: hexagonal point group ops!
		if(pPtOps && pHexPtOps)
			prop.put(strPath + "bns.ops.R" + std::to_string(iPtOp+1), std::get<1>((*pPtOps)[iOperBNS-1]));
		else
			prop.put(strPath + "bns.ops.R" + std::to_string(iPtOp+1), iOperBNS);
		prop.put(strPath + "bns.ops.v" + std::to_string(iPtOp+1), vecBNS);
		prop.put(strPath + "bns.ops.d" + std::to_string(iPtOp+1), numBNS);
		prop.put(strPath + "bns.ops.t" + std::to_string(iPtOp+1), itInvBNS);
	}

	std::size_t iNumLattVecsBNS = get_num<std::size_t>(istr);
	for(std::size_t iVec=0; iVec<iNumLattVecsBNS; ++iVec)
	{
		t_vec vecBNS = get_vector(3, istr);
		t_real numBNS = get_num<t_real>(istr);

		prop.put(strPath + "bns.latt.v" + std::to_string(iVec+1), vecBNS);
		prop.put(strPath + "bns.latt.d" + std::to_string(iVec+1), numBNS);
	}

	std::size_t iNumWycBNS = get_num<std::size_t>(istr);
	for(std::size_t iWyc=0; iWyc<iNumWycBNS; ++iWyc)
	{
		std::size_t iNumPos = get_num<std::size_t>(istr);
		std::size_t iMult = get_num<std::size_t>(istr);
		std::string strWycName = get_string(istr);

		prop.put(strPath + "bns.wyc.site" + std::to_string(iWyc+1) + ".name", strWycName);
		prop.put(strPath + "bns.wyc.site" + std::to_string(iWyc+1) + ".mult", iMult);

		for(std::size_t iPos=0; iPos<iNumPos; ++iPos)
		{
			t_vec vecWyc = get_vector(3, istr);
			t_real numWyc = get_num<t_real>(istr);
			t_mat matWycXYZ = get_matrix(3, 3, istr);
			t_mat matWycMXMYMZ = get_matrix(3, 3, istr);

			prop.put(strPath + "bns.wyc.site" + std::to_string(iWyc+1)
				+ ".pos" + std::to_string(iPos+1) + ".v", vecWyc);
			prop.put(strPath + "bns.wyc.site" + std::to_string(iWyc+1)
				+ ".pos" + std::to_string(iPos+1) + ".d", numWyc);
			prop.put(strPath + "bns.wyc.site" + std::to_string(iWyc+1)
				+ ".pos" + std::to_string(iPos+1) + ".R", matWycXYZ);
			prop.put(strPath + "bns.wyc.site" + std::to_string(iWyc+1)
				+ ".pos" + std::to_string(iPos+1) + ".M", matWycMXMYMZ);
		}
	}


	if(iTy == 4)	// OG
	{
		std::size_t iNumPtOpsOG = get_num<std::size_t>(istr);
		for(std::size_t iPtOp=0; iPtOp<iNumPtOpsOG; ++iPtOp)
		{
			std::size_t iOperOG = get_num<std::size_t>(istr);
			t_vec vecOG = get_vector(3, istr);
			t_real numOG = get_num<t_real>(istr);
			int itInvOG = get_num<int>(istr);

			// TODO: hexagonal point group ops!
			if(pPtOps && pHexPtOps)
				prop.put(strPath + "og.ops.R" + std::to_string(iPtOp+1), std::get<1>((*pPtOps)[iOperOG-1]));
			else
				prop.put(strPath + "og.ops.R" + std::to_string(iPtOp+1), iOperOG);
			prop.put(strPath + "og.ops.v" + std::to_string(iPtOp+1), vecOG);
			prop.put(strPath + "og.ops.d" + std::to_string(iPtOp+1), numOG);
			prop.put(strPath + "og.ops.t" + std::to_string(iPtOp+1), itInvOG);
		}

		std::size_t iNumLattVecsOG = get_num<std::size_t>(istr);
		for(std::size_t iVec=0; iVec<iNumLattVecsOG; ++iVec)
		{
			t_vec vecOG = get_vector(3, istr);
			t_real numOG = get_num<t_real>(istr);

			prop.put(strPath + "og.latt.v" + std::to_string(iVec+1), vecOG);
			prop.put(strPath + "og.latt.d" + std::to_string(iVec+1), numOG);
		}

		std::size_t iNumWycOG = get_num<std::size_t>(istr);
		for(std::size_t iWyc=0; iWyc<iNumWycOG; ++iWyc)
		{
			std::size_t iNumPos = get_num<std::size_t>(istr);
			std::size_t iMult = get_num<std::size_t>(istr);
			std::string strWycName = get_string(istr);

			prop.put(strPath + "og.wyc.site" + std::to_string(iWyc+1) + ".name", strWycName);
			prop.put(strPath + "og.wyc.site" + std::to_string(iWyc+1) + ".mult", iMult);

			for(std::size_t iPos=0; iPos<iNumPos; ++iPos)
			{
				t_vec vecWyc = get_vector(3, istr);
				t_real numWyc = get_num<t_real>(istr);
				t_mat matWycXYZ = get_matrix(3, 3, istr);
				t_mat matWycMXMYMZ = get_matrix(3, 3, istr);

				prop.put(strPath + "og.wyc.site" + std::to_string(iWyc+1)
					+ ".pos" + std::to_string(iPos+1) + ".v", vecWyc);
				prop.put(strPath + "og.wyc.site" + std::to_string(iWyc+1)
					+ ".pos" + std::to_string(iPos+1) + ".d", numWyc);
				prop.put(strPath + "og.wyc.site" + std::to_string(iWyc+1)
					+ ".pos" + std::to_string(iPos+1) + ".R", matWycXYZ);
				prop.put(strPath + "og.wyc.site" + std::to_string(iWyc+1)
					+ ".pos" + std::to_string(iPos+1) + ".M", matWycMXMYMZ);
			}
		}
	}

	//std::cout << strSGBNS << ", " << strSGOG << "\n";
	//std::cout << "."; std::cout.flush();
}


void convert_table(const char* pcFile)
{
	std::ifstream istr(pcFile);
	if(!istr)
	{
		std::cerr << "Cannot open \"" << pcFile << "\".\n";
		return;
	}

	std::vector<std::tuple<std::string, t_mat>> vecPtOps, vecHexPtOps;

	// read 48 point group operators
	std::cout << "Reading point group operators...\n";
	for(std::size_t i=0; i<48; ++i)
		vecPtOps.emplace_back(get_pointgroup_op(istr));

	// read 24 hexagonal point group operators
	std::cout << "Reading hexagonal point group operators...\n";
	for(std::size_t i=0; i<24; ++i)
		vecHexPtOps.emplace_back(get_pointgroup_op(istr));


	ptree::ptree prop;

	// convert the 1651 3D space groups
	std::cout << "Converting space groups...\n";
	for(std::size_t i=0; i<1651; ++i)
	{
		std::string strPath = "mag_groups.sg" + std::to_string(i+1) + ".";
		convert_spacegroup(istr, prop, strPath, &vecPtOps, &vecHexPtOps);
	}


	ptree::write_xml("maggroups.xml", prop,
		std::locale(),
		ptree::xml_writer_make_settings('\t', 1, std::string("utf-8")));
}


int main()
{
	convert_table("mag.dat");

	return 0;
}
