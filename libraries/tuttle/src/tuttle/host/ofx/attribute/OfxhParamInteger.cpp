#include "OfxhParamInteger.hpp"

namespace tuttle {
namespace host {
namespace ofx {
namespace attribute {


void OfxhParamInteger::derive( const OfxTime time, int& ) const OFX_EXCEPTION_SPEC
{
	throw OfxhException( kOfxStatErrUnsupported );
}

void OfxhParamInteger::integrate( const OfxTime time1, const OfxTime time2, int& ) const OFX_EXCEPTION_SPEC
{
	throw OfxhException( kOfxStatErrUnsupported );
}

/**
 * implementation of var args function
 */
void OfxhParamInteger::getV( va_list arg ) const OFX_EXCEPTION_SPEC
{
	int* value = va_arg( arg, int* );

	return get( *value );
}

/**
 * implementation of var args function
 */
void OfxhParamInteger::getV( const OfxTime time, va_list arg ) const OFX_EXCEPTION_SPEC
{
	int* value = va_arg( arg, int* );

	return get( time, *value );
}

/**
 * implementation of var args function
 */
void OfxhParamInteger::setV( va_list arg ) OFX_EXCEPTION_SPEC
{
	int value = va_arg( arg, int );

	return set( value );
}

/**
 * implementation of var args function
 */
void OfxhParamInteger::setV( const OfxTime time, va_list arg ) OFX_EXCEPTION_SPEC
{
	int value = va_arg( arg, int );

	return set( time, value );
}

/**
 * implementation of var args function
 */
void OfxhParamInteger::deriveV( const OfxTime time, va_list arg ) const OFX_EXCEPTION_SPEC
{
	int* value = va_arg( arg, int* );

	return derive( time, *value );
}

/**
 * implementation of var args function
 */
void OfxhParamInteger::integrateV( const OfxTime time1, const OfxTime time2, va_list arg ) const OFX_EXCEPTION_SPEC
{
	int* value = va_arg( arg, int* );

	return integrate( time1, time2, *value );
}



}
}
}
}
