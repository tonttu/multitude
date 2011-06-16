/* COPYRIGHT
*
* This file is part of Nimble.
*
* Copyright: MultiTouch Oy, Helsinki University of Technology and others.
*
* See file "Nimble.hpp" for authors and more details.
*
* This file is licensed under GNU Lesser General Public
* License (LGPL), version 2.1. The LGPL conditions can be found in 
* file "LGPL.txt" that is distributed with this source package or obtained 
* from the GNU organization (www.gnu.org).
* 
*/


#include "Matrix2.hpp"
#include "Matrix3.hpp"
#include "Matrix4.hpp"

namespace Nimble
{

	const Matrix2T<float> Matrix2T<float>::IDENTITY(
		1, 0, 
		0, 1);

	const Matrix2T<double> Matrix2T<double>::IDENTITY(
		1, 0, 
		0, 1);

	const Matrix3T<float> Matrix3T<float>::IDENTITY(
		1, 0, 0,
		0, 1, 0,
		0, 0, 1);

	const Matrix3T<double> Matrix3T<double>::IDENTITY(
		1, 0, 0,
		0, 1, 0,
		0, 0, 1);

	const Matrix4T<float> Matrix4T<float>::IDENTITY(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);

	const Matrix4T<double> Matrix4T<double>::IDENTITY(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
}


//template class Nimble::Matrix2T<float>;
//template class Nimble::Matrix2T<double>;
//
//template class Nimble::Matrix3T<float>;
//template class Nimble::Matrix3T<double>;
//
//template class Nimble::Matrix4T<float>;
//template class Nimble::Matrix4T<double>;
//
