/**
 * magnetic space group library
 * @author Tobias Weber
 * @date 7-apr-18
 * @license see 'LICENSE.EUPL' file
 */

#include "magsg.h"


// ----------------------------------------------------------------------------
// test: g++ -std=c++17 -fconcepts -o magsg libs/magsg.cpp -lboost_iostreams

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
