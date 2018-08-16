/**
 * polarisation test
 * @author Tobias Weber
 * @date aug-18
 * @license: see 'LICENSE.EUPL' file
 * 
 * g++ -std=c++2a -fconcepts -o pol pol.cpp
 */


#include <iostream>
#include "math_algos.h"
#include "math_conts.h"
using namespace m;
using namespace m_ops;

using t_real = double;
using t_cplx = std::complex<t_real>;
using t_vec = std::vector<t_cplx>;
using t_mat = mat<t_cplx, std::vector>;


int main()
{
	t_vec Mperp = create<t_vec>({0, 0, t_cplx(0,1)});
	t_vec P = create<t_vec>({0, 0, 1});
	t_cplx N(0,1);

	auto [I, P_f] = blume_maleev<t_vec, t_cplx>(P, Mperp, N);
	std::cout << "I = " << I << "\nP_f = " << P_f << std::endl;

	auto [I2, P_f2] = blume_maleev_indir<t_mat, t_vec, t_cplx>(P, Mperp, N);
	std::cout << "I2 = " << I2 << "\nP_f2 = " << P_f2 << std::endl;
	
	std::cout << "density matrix = " << pol_density_mat<t_vec, t_mat>(P) << std::endl;
	return 0;
}
