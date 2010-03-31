#include "SplinesImpl.hpp"

#include <Nimble/Vector3.hpp>

template Nimble::Vector3 Nimble::Splines::evalCatmullRom(float t, const std::vector<Nimble::Vector3> & cp, size_t index);
