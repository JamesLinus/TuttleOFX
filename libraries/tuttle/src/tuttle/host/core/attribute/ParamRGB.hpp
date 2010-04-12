#ifndef _TUTTLE_HOST_CORE_ATTRIBUTE_PARAMRGB_HPP_
#define _TUTTLE_HOST_CORE_ATTRIBUTE_PARAMRGB_HPP_

#include "ParamDouble.hpp"
#include <tuttle/host/ofx/attribute/OfxhMultiDimParam.hpp>
#include <tuttle/host/core/ImageEffectNode.hpp>

namespace tuttle {
namespace host {
namespace core {


class ParamRGB : public ofx::attribute::OfxhMultiDimParam<ParamDouble, 3 >
{
protected:
	ImageEffectNode& _effect;

	OfxRGBColourD _value;

public:
	ParamRGB( ImageEffectNode& effect, const std::string& name, const ofx::attribute::OfxhParamDescriptor& descriptor );
	ParamRGB* clone() const { return new ParamRGB( *this ); }

	OfxRGBColourD getDefault() const;
	void get( double& r, double& g, double& b ) const OFX_EXCEPTION_SPEC;
	void get( const OfxTime time, double& r, double& g, double& b ) const OFX_EXCEPTION_SPEC;
	void set( const double &r, const double &g, const double &b ) OFX_EXCEPTION_SPEC;
	void set( const OfxTime time, const double &r, const double &g, const double &b ) OFX_EXCEPTION_SPEC;
	void copy( const ParamRGB& p ) OFX_EXCEPTION_SPEC
	{
		_value = p._value;
	}
	void copy( const OfxhParam& p ) OFX_EXCEPTION_SPEC
	{
		const ParamRGB& param = dynamic_cast<const ParamRGB&>(p);
		copy( param );
	}
};

}
}
}

#endif