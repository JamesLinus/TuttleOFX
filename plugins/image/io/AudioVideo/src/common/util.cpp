#include "util.hpp"

#include <writer/AVWriterDefinitions.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

#include <boost/foreach.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/bind/bind.hpp>

#include <iostream>
#include <limits>

namespace tuttle {
namespace plugin {
namespace av {
namespace common {

LibAVParams::LibAVParams( const std::string& prefixScope, const int flags, const bool isDetailled )
	: _paramOFX()
	, _paramFlagOFXPerOption()
	, _childsPerChoice()
	, _avOptions()
	, _avContext( NULL )
{
	if( ! isDetailled )
	{
		if( prefixScope == kPrefixFormat )
		{
			 // Allocate an AVFormatContext
			_avContext = avformat_alloc_context();
		}
		else
		{
			// Allocate an AVCodecContext (NULL: the codec-specific defaults won't be initialized)
			_avContext = avcodec_alloc_context3( NULL );
		}

		avtranscoder::OptionArray commonOptions;
		avtranscoder::loadOptions( commonOptions, _avContext, flags );
		_avOptions.insert( std::make_pair( "", commonOptions ) );
	}
	else
	{
		if( prefixScope == kPrefixFormat )
		{
			_avOptions = avtranscoder::getOutputFormatOptions();
		}
		else if( prefixScope == kPrefixVideo )
		{
			_avOptions = avtranscoder::getVideoCodecOptions();
		}
		else if( prefixScope == kPrefixAudio )
		{
			_avOptions = avtranscoder::getAudioCodecOptions();
		}
	}
}

LibAVParams::~LibAVParams()
{
	av_free( _avContext );
}

void LibAVParams::fetchLibAVParams( OFX::ImageEffect& plugin, const std::string& prefix )
{
	// iterate on map keys
	BOOST_FOREACH( avtranscoder::OptionArrayMap::value_type& subGroupOption, _avOptions )
	{
		const std::string detailledName = subGroupOption.first;
		avtranscoder::OptionArray& options = subGroupOption.second;
				
		// iterate on options
		BOOST_FOREACH( avtranscoder::Option& option, options )
		{
			std::string name = prefix;
			if( ! detailledName.empty() )
			{
				name += detailledName;
				name += "_";
			}
			name += option.getName();
			
			switch( option.getType() )
			{
				case avtranscoder::eOptionBaseTypeBool:
				{
					_paramOFX.push_back( plugin.fetchBooleanParam( name ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeInt:
				{
					_paramOFX.push_back( plugin.fetchIntParam( name ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeDouble:
				{
					_paramOFX.push_back( plugin.fetchDoubleParam( name ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeString:
				{
					_paramOFX.push_back( plugin.fetchStringParam( name ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeRatio:
				{
					_paramOFX.push_back( plugin.fetchInt2DParam( name ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeChoice:
				{
					// manage exception of video/audio threads parameter: we want to manipulate an OFX Int parameter
					if( name == kPrefixVideo + kOptionThreads ||
						name == kPrefixAudio + kOptionThreads )
					{
						_paramOFX.push_back( plugin.fetchIntParam( name ) );
						break;
					}

					OFX::ChoiceParam* choice = plugin.fetchChoiceParam( name );
					_paramOFX.push_back( choice );
					_childsPerChoice.insert( std::make_pair( choice, std::vector<std::string>() ) );
					BOOST_FOREACH( const avtranscoder::Option& child, option.getChilds() )
					{
						_childsPerChoice.at( choice ).push_back( child.getName() );
					}
					break;
				}
				case avtranscoder::eOptionBaseTypeGroup:
				{
					_paramFlagOFXPerOption.insert( std::make_pair( option.getName(), std::vector<OFX::BooleanParam*>() ) );
					BOOST_FOREACH( const avtranscoder::Option& child, option.getChilds() )
					{
						std::string childName = prefix;
						if( ! detailledName.empty() )
						{
							childName += detailledName;
							childName += "_";
						}
						childName += child.getUnit();
						childName += common::kPrefixFlag;
						childName += child.getName();

						_paramFlagOFXPerOption.at( option.getName() ).push_back( plugin.fetchBooleanParam( childName ) );
					}
					break;
				}
			default:
				break;
			}
		}
	}
}

avtranscoder::ProfileLoader::Profile LibAVParams::getCorrespondingProfile( const std::string& detailledName )
{
	LibAVOptions optionsNameAndValue;

	// Get all libav options and values corresponding to the OFX parameters
	BOOST_FOREACH( OFX::ValueParam* param, _paramOFX )
	{
		// skip detailled params which does not concern our current format/codec
		if( ! detailledName.empty() && param->getName().find( "_" + detailledName + "_" ) == std::string::npos )
			continue;

		const std::string libavOptionName( getOptionNameWithoutPrefix( param->getName(), detailledName ) );
		const avtranscoder::Option& libavOption = getOption( libavOptionName, detailledName );
		std::string libavOptionValue( "" );

		// Manage OFX Boolean
		OFX::BooleanParam* paramBoolean = dynamic_cast<OFX::BooleanParam*>( param );
		if( paramBoolean )
		{
			if( libavOption.getDefaultBool() == libavOption.getBool() )
				continue;

			libavOptionValue = boost::to_string( paramBoolean->getValue() );
			optionsNameAndValue.insert( std::make_pair( libavOptionName, libavOptionValue ) );
			continue;
		}

		// Manage OFX Int
		OFX::IntParam* paramInt = dynamic_cast<OFX::IntParam*>( param );
		if( paramInt )
		{
			if( libavOption.getDefaultInt() == libavOption.getInt() )
				continue;

			libavOptionValue = boost::to_string( paramInt->getValue() );
			optionsNameAndValue.insert( std::make_pair( libavOptionName, libavOptionValue ) );
			continue;
		}

		// Manage OFX Double
		OFX::DoubleParam* paramDouble = dynamic_cast<OFX::DoubleParam*>( param );
		if( paramDouble )
		{
			if( boost::test_tools::check_is_close( libavOption.getDefaultDouble(), libavOption.getDouble(), boost::test_tools::percent_tolerance( 0.5 ) ) )
				continue;

			libavOptionValue = boost::to_string( paramDouble->getValue() );
			optionsNameAndValue.insert( std::make_pair( libavOptionName, libavOptionValue ) );
			continue;
		}

		// Manage OFX String
		OFX::StringParam* paramString = dynamic_cast<OFX::StringParam*>( param );
		if( paramString )
		{
			// skip empty string
			libavOptionValue = paramString->getValue();
			if( libavOptionValue.empty() )
				continue;

			if( libavOption.getDefaultString() == libavOption.getString() )
				continue;

			optionsNameAndValue.insert( std::make_pair( libavOptionName, libavOptionValue ) );
			continue;
		}

		// Manage OFX Int2D
		OFX::Int2DParam* paramRatio = dynamic_cast<OFX::Int2DParam*>( param );
		if( paramRatio )
		{
			if( libavOption.getDefaultRatio() == libavOption.getRatio() )
				continue;

			libavOptionValue = boost::to_string( paramRatio->getValue().x ) + ":" + boost::to_string( paramRatio->getValue().y );
			optionsNameAndValue.insert( std::make_pair( libavOptionName, libavOptionValue ) );
			continue;
		}

		// Manage OFX Choice
		OFX::ChoiceParam* paramChoice = dynamic_cast<OFX::ChoiceParam*>( param );
		if( paramChoice )
		{
			if( libavOption.getDefaultInt() == libavOption.getInt() )
				continue;

			size_t optionIndex = paramChoice->getValue();
			std::vector<std::string> childs( _childsPerChoice.at( paramChoice ) );
			if( childs.size() > optionIndex )
			{
				libavOptionValue = childs.at( optionIndex );
				optionsNameAndValue.insert( std::make_pair( libavOptionName, libavOptionValue ) );
			}
			continue;
		}
	}

	// Get all libav flags and values corresponding to the OFX parameters
	BOOST_FOREACH( FlagOFXPerOption::value_type& flagsPerOption, _paramFlagOFXPerOption )
	{
		try
		{
			const avtranscoder::Option& libavOption = getOption( flagsPerOption.first, detailledName );
			const std::string flagName( libavOption.getName() );

			// iterate on option's flags
			BOOST_FOREACH( OFX::BooleanParam* param, flagsPerOption.second )
			{
				// skip detailled flag params which does not concern our current format/codec
				if( ! detailledName.empty() && param->getName().find( "_" + detailledName + "_" ) == std::string::npos )
					continue;

				// get flag value
				std::string libavOptionValue( "" );
				if( param->getValue() )
					libavOptionValue.append( "+" );
				else
					libavOptionValue.append( "-" );
				libavOptionValue.append( getOptionNameWithoutPrefix( param->getName(), detailledName ) );

				// if first flag with this flagName
				if( optionsNameAndValue.find( flagName ) == optionsNameAndValue.end() )
				{
					// create new option in profile
					optionsNameAndValue.insert( std::make_pair( flagName, libavOptionValue ) );
				}
				else
				{
					// append flag to the existing option
					optionsNameAndValue.at( flagName ) += libavOptionValue;
				}
			}
		}
		catch( std::exception& e )
		{
			// std::cout << "can't create option from name " << flagsPerOption.first << " (" << detailledName << ")" << std::endl;
		}
	}

	// Create the corresponding profile
	avtranscoder::ProfileLoader::Profile profile;
	BOOST_FOREACH( LibAVOptions::value_type& nameAndValue, optionsNameAndValue )
	{
		profile[ nameAndValue.first ] = nameAndValue.second;
	}
	return profile;
}

avtranscoder::Option& LibAVParams::getOption(const std::string& libAVOptionName, const std::string& detailledName )
{
	avtranscoder::OptionArray& optionsArray = _avOptions.at( detailledName );

	avtranscoder::OptionArray::iterator iterOption = std::find_if( optionsArray.begin(), optionsArray.end(), boost::bind( &avtranscoder::Option::getName, _1) == libAVOptionName );
	const size_t optionIndex = std::distance( optionsArray.begin(), iterOption);

	if( optionIndex >= optionsArray.size() )
		BOOST_THROW_EXCEPTION( exception::Failed() << exception::user() + "option is not in array of detailled:  " + detailledName ); 

	return optionsArray.at( optionIndex );
}

void LibAVParams::setOption( const std::string& libAVOptionName, const std::string& value,  const std::string& prefix, const std::string& detailledName )
{
	try
	{
		// Get libav option
		avtranscoder::Option& option = getOption( libAVOptionName, detailledName );

		// Set libav option's value
		option.setString( value );

		// Get corresponding OFX parameter
		OFX::ValueParam* param = getOFXParameter( libAVOptionName, detailledName );
		if( ! param)
		{
			TUTTLE_TLOG( TUTTLE_INFO, "Can't get OFX parameter corresponding to option " << libAVOptionName << " of subgroup " << detailledName );
		}

		// Set OFX parameter's value
		OFX::BooleanParam* paramBoolean = dynamic_cast<OFX::BooleanParam*>( param );
		if( paramBoolean )
		{
			paramBoolean->setValue( option.getBool() );
			return;
		}
		OFX::IntParam* paramInt = dynamic_cast<OFX::IntParam*>( param );
		if( paramInt )
		{
			paramInt->setValue( option.getInt() );
			return;
		}
		OFX::DoubleParam* paramDouble = dynamic_cast<OFX::DoubleParam*>( param );
		if( paramDouble )
		{
			paramDouble->setValue( option.getDouble() );
			return;
		}
		OFX::StringParam* paramString = dynamic_cast<OFX::StringParam*>( param );
		if( paramString )
		{
			paramString->setValue( option.getString() );
			return;
		}
		OFX::Int2DParam* paramRatio = dynamic_cast<OFX::Int2DParam*>( param );
		if( paramRatio )
		{
			paramRatio->setValue( option.getRatio().first, option.getRatio().second );
			return;
		}
		OFX::ChoiceParam* paramChoice = dynamic_cast<OFX::ChoiceParam*>( param );
		if( paramChoice )
		{
			paramChoice->setValue( option.getInt() );
			return;
		}
	}
	catch( std::exception& e )
	{
		TUTTLE_TLOG( TUTTLE_INFO, "Can't set option " << libAVOptionName << " to " << value << ": " << e.what() );
	}
}

OFX::ValueParam* LibAVParams::getOFXParameter( const std::string& libAVOptionName, const std::string& detailledName )
{
	BOOST_FOREACH( OFX::ValueParam* param, _paramOFX )
	{
		if( getOptionNameWithoutPrefix( param->getName(), detailledName ) == libAVOptionName )
			return param;
	}
	return NULL;
}

void addOptionsToGroup( OFX::ImageEffectDescriptor& desc, OFX::GroupParamDescriptor* group, avtranscoder::OptionArray& optionsArray, const std::string& prefix, const std::string& detailledName )
{
	OFX::ParamDescriptor* param = NULL;
	BOOST_FOREACH( avtranscoder::Option& option, optionsArray )
	{
		std::string name = prefix;
		if( ! detailledName.empty() )
		{
			name += detailledName;
			name += "_";
		}
		name += option.getName();
		
		switch( option.getType() )
		{
			case avtranscoder::eOptionBaseTypeBool:
			{
				OFX::BooleanParamDescriptor* boolParam = desc.defineBooleanParam( name );
				boolParam->setDefault( option.getDefaultBool() );
				param = boolParam;
				break;
			}
			case avtranscoder::eOptionBaseTypeInt:
			{
				OFX::IntParamDescriptor* intParam = desc.defineIntParam( name );
				intParam->setDefault( option.getDefaultInt() );
				const int min = option.getMin() > std::numeric_limits<int>::min() ? option.getMin() : std::numeric_limits<int>::min();
				const int max = option.getMax() < std::numeric_limits<int>::max() ? option.getMax() : std::numeric_limits<int>::max();
				intParam->setRange( min, max );
				intParam->setDisplayRange( min, max );
				param = intParam;
				break;
			}
			case avtranscoder::eOptionBaseTypeDouble:
			{
				OFX::DoubleParamDescriptor* doubleParam = desc.defineDoubleParam( name );
				doubleParam->setDefault( option.getDefaultDouble() );
				doubleParam->setRange( option.getMin(), option.getMax() );
				doubleParam->setDisplayRange( option.getMin(), option.getMax() );
				param = doubleParam;
				break;
			}
			case avtranscoder::eOptionBaseTypeString:
			{
				OFX::StringParamDescriptor* strParam = desc.defineStringParam( name );
				strParam->setDefault( option.getDefaultString() );
				param = strParam;
				break;
			}
			case avtranscoder::eOptionBaseTypeRatio:
			{
				OFX::Int2DParamDescriptor* ratioParam = desc.defineInt2DParam( name );
				// @todo: minX, minY, maxX, maxY could be different
				ratioParam->setDefault( option.getDefaultRatio().first, option.getDefaultRatio().second );
				const int min = option.getMin() > std::numeric_limits<int>::min() ? option.getMin() : std::numeric_limits<int>::min();
				const int max = option.getMax() < std::numeric_limits<int>::max() ? option.getMax() : std::numeric_limits<int>::max();
				ratioParam->setRange( min, min, max, max );
				ratioParam->setDisplayRange( min, min, max, max );
				param = ratioParam;
				break;
			}
			case avtranscoder::eOptionBaseTypeChoice:
			{
				// manage exception of threads parameters: we want to manipulate an OFX Int parameter
				if( name == kPrefixVideo + kOptionThreads ||
					name == kPrefixAudio + kOptionThreads )
				{
					OFX::IntParamDescriptor* intParam = desc.defineIntParam( name );
					intParam->setDefault( 0 ); // 0 = threads auto (optimal)
					intParam->setRange( 0, std::numeric_limits<int>::max() );
					intParam->setDisplayRange( 0, 64 );
					param = intParam;
					break;
				}

				OFX::ChoiceParamDescriptor* choiceParam = desc.defineChoiceParam( name );
				choiceParam->setDefault( option.getDefaultChildIndex() );
				BOOST_FOREACH( const avtranscoder::Option& child, option.getChilds() )
				{
					choiceParam->appendOption( child.getName() + " " + child.getHelp() );
				}
				param = choiceParam;
				break;
			}
			case avtranscoder::eOptionBaseTypeGroup:
			{
				std::string groupName = prefix;
				groupName += kPrefixGroup;
				if( ! detailledName.empty() )
				{
					groupName += detailledName;
					groupName += "_";
				}
				groupName += option.getName();
				
				OFX::GroupParamDescriptor* groupParam = desc.defineGroupParam( groupName );
				groupParam->setOpen( false );
				BOOST_FOREACH( const avtranscoder::Option& child, option.getChilds() )
				{
					std::string childName = prefix;
					if( ! detailledName.empty() )
					{
						childName += detailledName;
						childName += "_";
					}
					childName += child.getUnit();
					childName += kPrefixFlag;
					childName += child.getName();
					
					OFX::BooleanParamDescriptor* param = desc.defineBooleanParam( childName );
					param->setLabel( child.getName() );
					param->setDefault( child.getOffset() );
					param->setHint( child.getHelp() );
					param->setParent( groupParam );
				}
				param = groupParam;
				break;
			}
			default:
				break;
		}
		if( param )
		{
			param->setLabel( option.getName() );
			param->setHint( option.getHelp() );
			param->setParent( group );

			// add help for our custom threads parameters
			if( name == kPrefixVideo + kOptionThreads ||
				name == kPrefixAudio + kOptionThreads )
			{
				param->setHint( "set a number of threads for encoding (0 autodetect a suitable number of threads to use)" );
			}
		}
	}
}

void addOptionsToGroup( OFX::ImageEffectDescriptor& desc, OFX::GroupParamDescriptor* group,  avtranscoder::OptionArrayMap& optionArrayMap, const std::string& prefix )
{
	// iterate on map keys
	BOOST_FOREACH( avtranscoder::OptionArrayMap::value_type& subGroupOption, optionArrayMap )
	{
		const std::string detailledName = subGroupOption.first;
		avtranscoder::OptionArray& options = subGroupOption.second;
		
		addOptionsToGroup( desc, group, options, prefix, detailledName );
	}
}

std::string getOptionNameWithoutPrefix( const std::string& optionName, const std::string& detailledName )
{
	std::string nameWithoutPrefix( optionName );
	
	// prefix
	if( nameWithoutPrefix.find( kPrefixFormat ) != std::string::npos )
		nameWithoutPrefix.erase( 0, kPrefixFormat.size() );
	else if( nameWithoutPrefix.find( kPrefixVideo ) != std::string::npos )
		nameWithoutPrefix.erase( 0, kPrefixVideo.size() );
	else if( nameWithoutPrefix.find( kPrefixAudio ) != std::string::npos )
		nameWithoutPrefix.erase( 0, kPrefixAudio.size() );
	
	// detailled group name
	if( ! detailledName.empty() && nameWithoutPrefix.find( detailledName ) != std::string::npos )
	{
		// detailledName.size() + 1: also remove the "_"
		nameWithoutPrefix.erase( 0, detailledName.size() + 1 );
	}
	
	// childs of groups (flag)
	size_t endedPosition;
	if( ( endedPosition = nameWithoutPrefix.find( kPrefixFlag ) ) != std::string::npos )
	{
		nameWithoutPrefix.erase( 0, endedPosition + kPrefixFlag.size() );
	}
	
	return nameWithoutPrefix;
}


std::string getOptionFlagName( const std::string& optionName, const std::string& detailledName )
{
	std::string flagName;
	
	if( optionName.find( kPrefixFlag ) != std::string::npos )
	{
		size_t startedPosition;
		if( detailledName.empty() )
			startedPosition = optionName.find( "_" );
		else
		{
			startedPosition = optionName.find( "_", prefixSize );
		}
		++startedPosition; // started after the "_"
		size_t endedPosition = optionName.find( kPrefixFlag, startedPosition );
		
		flagName = optionName.substr( startedPosition, endedPosition - startedPosition );
	}
	
	return flagName;
}

void disableOFXParamsForFormatOrCodec( OFX::ImageEffect& plugin, avtranscoder::OptionArrayMap& optionArrayMap, const std::string& filter, const std::string& prefix )
{
	// iterate on map keys
	BOOST_FOREACH( avtranscoder::OptionArrayMap::value_type& subGroupOption, optionArrayMap )
	{
		const std::string detailledName = subGroupOption.first;
		avtranscoder::OptionArray& options = subGroupOption.second;

		// iterate on options
		BOOST_FOREACH( avtranscoder::Option& option, options )
		{
			std::string name = prefix;
			name += detailledName;
			name += "_";
			name += option.getName();

			switch( option.getType() )
			{
				case avtranscoder::eOptionBaseTypeBool:
				{
					OFX::BooleanParam* curOpt = plugin.fetchBooleanParam( name );
					curOpt->setIsSecretAndDisabled( !( detailledName == filter ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeInt:
				{
					OFX::IntParam* curOpt = plugin.fetchIntParam( name );
					curOpt->setIsSecretAndDisabled( !( detailledName == filter ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeDouble:
				{
					OFX::DoubleParam* curOpt = plugin.fetchDoubleParam( name );
					curOpt->setIsSecretAndDisabled( !( detailledName == filter ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeString:
				{
					OFX::StringParam* curOpt = plugin.fetchStringParam( name );
					curOpt->setIsSecretAndDisabled( !( detailledName == filter ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeRatio:
				{
					OFX::Int2DParam* curOpt = plugin.fetchInt2DParam( name );
					curOpt->setIsSecretAndDisabled( !( detailledName == filter ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeChoice:
				{
					OFX::ChoiceParam* curOpt = plugin.fetchChoiceParam( name );
					curOpt->setIsSecretAndDisabled( !( detailledName == filter ) );
					break;
				}
				case avtranscoder::eOptionBaseTypeGroup:
				{
					std::string groupName = prefix;
					groupName += common::kPrefixGroup;
					groupName += detailledName;
					groupName += "_";
					groupName += option.getName();
					
					OFX::GroupParam* curOpt = plugin.fetchGroupParam( groupName );
					curOpt->setIsSecretAndDisabled( !( detailledName == filter ) );
					
					BOOST_FOREACH( const avtranscoder::Option& child, option.getChilds() )
					{
						std::string childName = prefix;
						if( ! detailledName.empty() )
						{
							childName += detailledName;
							childName += "_";
						}
						childName += child.getUnit();
						childName += common::kPrefixFlag;
						childName += child.getName();
						
						OFX::BooleanParam* curOpt = plugin.fetchBooleanParam( childName );
						curOpt->setIsSecretAndDisabled( !( detailledName == filter ) );
					}
					break;
				}
				default:
					break;
			}
		}
	}
}

}
}
}
}