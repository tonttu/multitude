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

#ifndef NIMBLE_QUATERNION_H
#define NIMBLE_QUATERNION_H

#include <math.h>
#include "Vector3.hpp"
#include "Matrix3.hpp"
#include "Matrix4.hpp"

namespace Nimble {

  /// A quaternion class
  /** Quaternion are typically used to present 3D rotations in a way that can be
      easily interpolated, and which is not susceptible to the artifacts that plague
      the "pitch/roll/yaw" definition.
  */
  template <typename T>
  class QuaternionT
  {
  public:
    /// The quaternion type
    typedef T type;

    /// The quaternion x element value
    T x;
    /// The quaternion y element value
    T y;
    /// The quaternion z element value
    T z;
    /// The quaternion w element value
    T w;

    /// Constructs a quaternion object, with uninitialized values
    QuaternionT() {}
    /// Constructs a quaternion object, with values copied from another quaternion object
    /// @param o The quaternion to copy
    QuaternionT(const QuaternionT<T> & o) : x(o.x), y(o.y), z(o.z), w(o.w) {}

    /// Constructs a quaternion object, with values copied from a 3D vector and a float
    /// @param v The 3D vector that defines the x, y, z value for this uaternion
    /// @param w_ The w value
    QuaternionT(const Vector3T<T> & v, T w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

    /// Constructs a quaternion with given values
    QuaternionT(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}

    /// Converts a rotation matrix to quaternion format
    QuaternionT(const Matrix3T<T> & m)
    {
      *this = m;
    }

    QuaternionT & normalize()
    {
      float m = Nimble::Math::InvSqrt(lensq());
      x *= m;
      y *= m;
      z *= m;
      w *= m;
      return *this;
    }

    /// Transforms the argument vector this quaternion transformation
    template <typename Y>
    Vector3T<Y> operator*(const Vector3T<Y> & v) const
    {
      // nVidia SDK implementation
      Vector3T<Y> qvec(x, y, z);
      Vector3T<Y> uv = ::cross(qvec, v);
      Vector3T<Y> uuv = ::cross(qvec, uv);
      uv *= (2.0f * w);
      uuv *= 2.0f;
      return v + uv + uuv;
    }

    /// Addition operator
    QuaternionT operator+(const QuaternionT & v)
    {
      return QuaternionT(x+v.x, y+v.y, z+v.z, w+v.w);
    }
    /// Subtraction operator
    QuaternionT operator-(const QuaternionT & v)
    {
      return QuaternionT(x-v.x, y-v.y, z-v.z, w-v.w);
    }
    /// Accumulation add operator
    QuaternionT & operator+=(const QuaternionT & v)
    {
      x += v.x; y += v.y; z += v.z; w += v.w; return *this;
    }
    /// Accumulation minus operator
    QuaternionT & operator-=(const QuaternionT & v)
    {
      x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this;
    }
    /// Multiply the quaternion components
    /// @param v The multiplier
    QuaternionT & operator*=(T v)
    {
      x *= v; y *= v; z *= v; w *= v; return *this;
    }
    /// Multiply this quaterion with another, and store the result into this quaternion
    QuaternionT & operator*=(const QuaternionT & v)
    {
      T nx = y*v.z-z*v.y+w*v.x+x*v.w;
      T ny = z*v.x-x*v.z+w*v.y+y*v.w;
      T nz = x*v.y-y*v.x+w*v.z+z*v.w;
      w = w*v.w-x*v.x-y*v.y-z*v.z;
      x = nx, y = ny, z = nz;
      return *this;
    }
    /// Multiply the components of this quaternion directly with the components of another quaternion
    QuaternionT & operator^=(const QuaternionT & v)
    {
      x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this;
    }
    /// Negate a quaternion
    /// @return Returns the values of this quaternion, multiplied with -1
    const QuaternionT operator-() const
    {
      return QuaternionT(-x,-y,-z,-w);
    }

    /// Negate the x, y, and z value of this quaternion
    /** @return Returns the x, y, z, values of this quaternion, multiplied with -1, and the
        w value as such.
      */
    const QuaternionT operator~() const
    {
      return QuaternionT(-x,-y,-z,w);
    }

    /// Extracts the rotation part of a 4x4 matrix, and calculates the quaternion values from that
    void operator=(const Matrix4T<T> & m)
    {
      *this = m.getRotation();
    }

    /// Calculates quaternion values from a 3x3 rotation matrix
    /** @param m The matrix to be copied. */
    void operator=(const Matrix3T<T> & m)
    {
      Vector3T<T> axis;
      T angle;
      m.getRotateAroundAxis(axis, angle);
      angle *= T(0.5);
      axis.normalize();

      T si = Math::Sin(angle);
      w = Math::Cos(angle);
      x = axis.x * si;
      y = axis.y * si;
      z = axis.z * si;

    }

    /// The squared length of this quaterion
    /** This function returns x*x+y*y+z*z+w*w. */
    T lensq(void) const
    {
      return(x*x+y*y+z*z+w*w);
    }
    /// Returns dot product between this quatertion and the argument quaternion
    T dotp(const QuaternionT & v) const
    {
      return(x*v.x+y*v.y+z*v.z+w*v.w);
    }

    /// Converts this quaternion into a 3x3 rotation matrix
    operator Nimble::Matrix3T<T>() const
    {
      Nimble::Matrix3T<T> m;

      float tx = 2.0f * x;
      float ty = 2.0f * y;
      float tz = 2.0f * z;
      float twx = tx * w;
      float twy = ty * w;
      float twz = tz * w;
      float txx = tx * x;
      float txy = ty * x;
      float txz = tz * x;
      float tyy = ty * y;
      float tyz = tz * y;
      float tzz = tz * z;

      m[0][0] = 1.0f - (tyy + tzz);
      m[0][1] = txy - twz;
      m[0][2] = txz + twy;
      m[1][0] = txy + twz;
      m[1][1] = 1.0f - (txx + tzz);
      m[1][2] = tyz - twx;
      m[2][0] = txz - twy;
      m[2][1] = tyz + twx;
      m[2][2] = 1.0f - (txx + tyy);

      return m;
    }

    /// Converts this quaternion into a 4x4 matrix
    /** @return The top-left 3x3 components of the matrix are set to a rotation matrix.
        The remaining components are from an identity matrix. */
    operator Nimble::Matrix4T<T>() const
    {
      Nimble::Matrix4T<T> m;
      m.identity();
      m.setRotation(*this);
      return m;
    }
/*
    Matrix33 Rmatrix() const {
      Matrix33 ret;
      T leninv = lensq();
      if(leninv==0)
        return(Matrix33(0));
      else
        leninv=(T)1.0/(T)sqrt(leninv);

      T s2 = s*s, x2 = x*x, y2 = y*y, z2 = z*z;
      ret.C[0]=(s2+x2-y2-z2);
      ret.C[4]=(s2-x2+y2-z2);
      ret.C[8]=(s2-x2-y2+z2);

      T sx = s*x, yz = y*z;
      ret.C[1*3+2]=2*(yz+sx);
      ret.C[2*3+1]=2*(yz-sx);
      T sy = s*y, zx = z*x;
      ret.C[0*3+2]=2*(zx-sy);
      ret.C[2*3+0]=2*(zx+sy);
      T sz = s*z, xy = x*y;
      ret.C[0*3+1]=2*(xy+sz);
      ret.C[1*3+0]=2*(xy-sz);
      return(ret);
    }

    void fillAffine33(Affine33 &aff) const {
      T leninv = lensq();
      if(leninv==0)
        return;
      else
        leninv=(T)1.0/(T)sqrt(leninv);

      T s2 = s*s, x2 = x*x, y2 = y*y, z2 = z*z;
      aff.C[0] =leninv*(s2+x2-y2-z2);
      aff.C[5] =leninv*(s2-x2+y2-z2);
      aff.C[10]=leninv*(s2-x2-y2+z2);

      leninv += leninv;

      T sx = s*x, yz = y*z;
      aff.C[1*4+2]=leninv*(yz+sx);
      aff.C[2*4+1]=leninv*(yz-sx);
      T sy = s*y, zx = z*x;
      aff.C[0*4+2]=leninv*(zx-sy);
      aff.C[2*4+0]=leninv*(zx+sy);
      T sz = s*z, xy = x*y;
      aff.C[0*4+1]=leninv*(xy+sz);
      aff.C[1*4+0]=leninv*(xy-sz);
    }
*/
    /// Performs slerp interpolation between two quaternions
    static QuaternionT slerp(const QuaternionT & q1, QuaternionT q2, T t)
    {
      T sinom, scale0, scale1, theta;

      // calc cosine
      T cosom = Math::Clamp<T>(q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w,
                               -1.0, 1.0);

      // Check if the quaternions are on opposite hemispheres
      // and adjust the signs if this is the case
      if(cosom < T(0.0)) {
        cosom = -cosom;
        q2 = -q2;
      }

      // Set the factors to do linear interpolation. A special case
      // when the two quaternions are very close
      scale0 = T(1.0) - t;
      scale1 = t;

      if((T(1.0) - cosom) > T(1e-03)) {
        // The quaternions aren't very close, proceed with SLERP

        theta = Math::ACos(cosom);
        sinom =	T(1.0) / Math::Sin(theta);
        scale0 = Math::Sin(theta * scale0) * sinom;
        scale1 = Math::Sin(theta * scale1) * sinom;
      }

      // Do the interpolation
      T ww = scale0 * q1.w + scale1 * q2.w;
      T xx = scale0 * q1.x + scale1 * q2.x;
      T yy = scale0 * q1.y + scale1 * q2.y;
      T zz = scale0 * q1.z + scale1 * q2.z;

      return QuaternionT(xx, yy, zz, ww);
    }

    /// Multiplies two quaterions.
    inline QuaternionT operator*(const QuaternionT & b)
    {
      return QuaternionT( y*b.z-z*b.y+w*b.x+x*b.w,
            z*b.x-x*b.z+w*b.y+y*b.w,
            x*b.y-y*b.x+w*b.z+z*b.w,
            w*b.w-x*b.x-y*b.y-z*b.z);
    }

    /// Converts this quaternion to angle/axis format
    void getAngleAxis(float & angle, Vector3T<T> & axis)
    {
      T len = x*x + y*y + z*z;
      if(len > T(0.0)) {
        T ilen = Nimble::Math::InvSqrt(len);
        angle = T(2.0) * Nimble::Math::ACos(w);
        axis.x = x*ilen;
        axis.y = y*ilen;
        axis.z = z*ilen;
      } else {
        angle = T(0);
        axis.make(1.0, 0.0, 0.0);
      }
    }

    /// Create a new quaternion based on rotation around an axis
    /// @param angle The rotation angle, in radians
    /// @param axis The axis, around which the rotation is performed
    static QuaternionT rotation(T angle, Vector3T<T> axis)
    {
      angle *= 0.5;
      axis.normalize();
      return QuaternionT(axis*Math::Sin(angle), Math::Cos(angle));
    }
  };

  /// Serialization operator for Quaternions
  template <typename T>
  inline std::ostream& operator<<(std::ostream& s, const QuaternionT<T> & v)
  {
    return s << "(" << v.x << " " << v.y << " " << v.z << " ;" << v.w << ")";
  }

  /// Quaternion of type float
  typedef QuaternionT<float> Quaternionf;
  /// Quaternion of type double
  typedef QuaternionT<double> Quaterniond;
  /// Default (float) quaternion type
  typedef QuaternionT<float> Quaternion;
}
#endif
