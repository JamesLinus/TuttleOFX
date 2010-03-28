#include "EXRWriterPluginFactory.hpp"
#include "EXRWriterPlugin.hpp"
#include "EXRWriterDefinitions.hpp"

#include <tuttle/plugin/ImageGilProcessor.hpp>
#include <tuttle/plugin/PluginException.hpp>

#include <string>
#include <iostream>
#include <stdio.h>
#include <cmath>
#include <ofxsImageEffect.h>
#include <ofxsMultiThread.h>
#include <boost/gil/gil_all.hpp>
#include <boost/scoped_ptr.hpp>

namespace tuttle {
namespace plugin {
namespace exr {
namespace writer {

static const bool kSupportTiles              = false;

/**
 * @brief Function called to describe the plugin main features.
 * @param[in, out]   desc     Effect descriptor
 */
void EXRWriterPluginFactory::describe( OFX::ImageEffectDescriptor& desc )
{
	desc.setLabels( "TuttleExrWriter", "ExrWriter",
	                "Exr file writer" );
	desc.setPluginGrouping( "tuttle" );

	// add the supported contexts
	desc.addSupportedContext( OFX::eContextGeneral );

	// add supported pixel depths
	desc.addSupportedBitDepth( OFX::eBitDepthUByte );
	desc.addSupportedBitDepth( OFX::eBitDepthUShort );
	desc.addSupportedBitDepth( OFX::eBitDepthFloat );

	// plugin flags
	desc.setSupportsMultiResolution( false );
	desc.setSupportsTiles( kSupportTiles );
}

/**
 * @brief Function called to describe the plugin controls and features.
 * @param[in, out]   desc       Effect descriptor
 * @param[in]        context    Application context
 */
void EXRWriterPluginFactory::describeInContext( OFX::ImageEffectDescriptor& desc,
                                                OFX::ContextEnum            context )
{
	OFX::ClipDescriptor* srcClip = desc.defineClip( kOfxImageEffectSimpleSourceClipName );
	// Exr only supports RGB(A)
	srcClip->addSupportedComponent( OFX::ePixelComponentRGBA );
	srcClip->setSupportsTiles( kSupportTiles );

	// Create the mandated output clip
	OFX::ClipDescriptor* dstClip = desc.defineClip( kOfxImageEffectOutputClipName );
	// Exr only supports RGB(A)
	dstClip->addSupportedComponent( OFX::ePixelComponentRGBA );
	dstClip->setSupportsTiles( kSupportTiles );

	// Controls
	OFX::StringParamDescriptor* filename = desc.defineStringParam( kOutputFilename );
	filename->setLabels( kOutputFilenameLabel, kOutputFilenameLabel, kOutputFilenameLabel );
	filename->setStringType( OFX::eStringTypeFilePath );
	filename->setCacheInvalidation( OFX::eCacheInvalidateValueAll );

	OFX::ChoiceParamDescriptor* bitDepth = desc.defineChoiceParam( kParamBitDepth );
	bitDepth->setLabels( kParamBitDepthLabel, kParamBitDepthLabel, "Output bit depth" );
	bitDepth->appendOption( "half float" );
	bitDepth->appendOption( "float" );
	bitDepth->appendOption( "32 bits" );
	bitDepth->setDefault( 0 );

	OFX::ChoiceParamDescriptor* componentsType = desc.defineChoiceParam( kParamComponentsType );
	componentsType->setLabels( kParamComponentsTypeLabel, kParamComponentsTypeLabel, "Output component type" );
	componentsType->appendOption( "gray" );
	componentsType->appendOption( "rgb" );
	componentsType->appendOption( "rgba" );
	componentsType->setDefault( 2 );

	OFX::PushButtonParamDescriptor* renderButton = desc.definePushButtonParam( kRender );
	renderButton->setScriptName( "renderButton" );
}

/**
 * @brief Function called to create a plugin effect instance
 * @param[in] handle  effect handle
 * @param[in] context    Application context
 * @return  plugin instance
 */
OFX::ImageEffect* EXRWriterPluginFactory::createInstance( OfxImageEffectHandle handle,
                                                          OFX::ContextEnum     context )
{
	return new EXRWriterPlugin( handle );
}

}
}
}
}
