#ifndef NIMBLE_QUATERNION_H
#define NIMBLE_QUATERNION_H

#include <math.h>
#include "Vector3.hpp"
#include "Matrix3.hpp"
#include "Matrix4.hpp"

namespace Nimble {

  template <typename T>
  class QuaternionT
  {
  public:
    typedef T type;

    T x,y,z,w;

    QuaternionT() {}
    QuaternionT(const QuaternionT<T> & o) : x(o.x), y(o.y), z(o.z), w(o.w) {}

    QuaternionT(const Vector3T<T> & v, T w_) : x(v.x), y(v.y), z(v.z), w(w_) {}

    QuaternionT(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) {}

    QuaternionT(const Matrix3T<T> & m)
    {
      *this = m;
    }

    template <typename Y>
    Vector3T<Y> operator*(const Vector3T<Y> & v) const
    {
      // nVidia SDK implementation
      Vector3T<Y> qvec(x, y, z);
      Vector3T<Y> uv = cross(qvec, v);
      Vector3T<Y> uuv = cross(qvec, uv);
      uv *= (2.0f * w);
      uuv *= 2.0f;
      return v + uv + uuv;
    }

    QuaternionT operator+(const QuaternionT & v)
    {
      return QuaternionT(x+v.x, y+v.y, z+v.z, w+v.w);
    }
    QuaternionT operator-(const QuaternionT & v)
    {
      return QuaternionT(x-v.x, y-v.y, z-v.z, w-v.w);
    }

    QuaternionT & operator+=(const QuaternionT & v)
    {
      x += v.x; y += v.y; z += v.z; w += v.w; return *this;
    }
    QuaternionT & operator-=(const QuaternionT & v)
    {
      x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this;
    }
    QuaternionT & operator*=(T v)
    {
      x *= v; y *= v; z *= v; w *= v; return *this;
    }
    QuaternionT & operator*=(const QuaternionT & v)
    {
      T nx = y*v.z-z*v.y+w*v.x+x*v.w;
      T ny = z*v.x-x*v.z+w*v.y+y*v.w;
      T nz = x*v.y-y*v.x+w*v.z+z*v.w;
      w = w*v.w-x*v.x-y*v.y-z*v.z;
      x = nx, y = ny, z = nz;
      return *this; 
    }
    QuaternionT & operator^=(const QuaternionT & v)
    {
      x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this;
    }
    const QuaternionT operator-() const
    {
      return QuaternionT(-x,-y,-z,-w);
    }

    const QuaternionT operator~() const
    {
      return QuaternionT(-x,-y,-z,w);
    }

    void operator=(const Matrix4T<T> & m)
    {
      *this = m.getRotation();
    }

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

    T lensq(void) const
    {
      return(x*x+y*y+z*z+w*w);
    }
    T dotp(const QuaternionT & v) const
    {
      return(x*v.x+y*v.y+z*v.z+w*v.w);
    }

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

    inline QuaternionT operator*(const QuaternionT & b)
    {
      return QuaternionT( y*b.z-z*b.y+w*b.x+x*b.w,
            z*b.x-x*b.z+w*b.y+y*b.w,
            x*b.y-y*b.x+w*b.z+z*b.w,
            w*b.w-x*b.x-y*b.y-z*b.z);
    }

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

    static QuaternionT rotation(T angle, Vector3T<T> axis)
    {
      angle *= 0.5;
      axis.normalize();
      return QuaternionT(Math::Cos(angle), axis*Math::Sin(angle));
    }
  };

  template <typename T>
  inline std::ostream& operator<<(std::ostream& s, const QuaternionT<T> & v)
  {
    return s << "(" << v.x << " " << v.y << " " << v.z << " ;" << v.w << ")";
  }

  typedef QuaternionT<float> Quaternionf;
  typedef QuaternionT<double> Quaterniond;
  typedef QuaternionT<float> Quaternion;
}
#endif
