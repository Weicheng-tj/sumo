/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    GNEAttributeCarrier.cpp
/// @author  Jakob Erdmann
/// @date    Feb 2011
/// @version $Id$
///
// Abstract Base class for gui objects which carry attributes
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#include <config.h>

#include <algorithm>
#include <utils/common/MsgHandler.h>
#include <utils/xml/SUMOSAXAttributes.h>
#include <utils/gui/images/GUIIconSubSys.h>
#include <utils/gui/globjects/GUIGlObject.h>
#include <utils/geom/GeomConvHelper.h>
#include <netbuild/NBEdge.h>
#include <netedit/netelements/GNEEdge.h>
#include <netedit/netelements/GNELane.h>

#include "GNEAttributeCarrier.h"
#include "GNEUndoList.h"
#include "GNENet.h"


// ===========================================================================
// static members
// ===========================================================================

std::map<SumoXMLTag, GNEAttributeCarrier::TagValues> GNEAttributeCarrier::myAllowedAttributes;

const std::string GNEAttributeCarrier::FEATURE_LOADED = "loaded";
const std::string GNEAttributeCarrier::FEATURE_GUESSED = "guessed";
const std::string GNEAttributeCarrier::FEATURE_MODIFIED = "modified";
const std::string GNEAttributeCarrier::FEATURE_APPROVED = "approved";
const double GNEAttributeCarrier::INVALID_POSITION = -1000000;


// ===========================================================================
// method definitions
// ===========================================================================

// ---------------------------------------------------------------------------
// GNEAttributeCarrier::TagValues - methods
// ---------------------------------------------------------------------------

GNEAttributeCarrier::TagValues::TagValues() :
    myTagProperty(TAGPROPERTY_NETELEMENT),
    myIcon(ICON_EMPTY),
    myPositionListed(0),
    myParentTag(SUMO_TAG_NOTHING),
    myMaxNumberOfChilds(0) {
}


GNEAttributeCarrier::TagValues::TagValues(int tagProperty, int positionListed, GUIIcon icon, SumoXMLTag parentTag, int maxNumberOfChilds) :
    myTagProperty(tagProperty),
    myIcon(icon),
    myPositionListed(positionListed),
    myParentTag(parentTag),
    myMaxNumberOfChilds(maxNumberOfChilds){
}


const std::map<SumoXMLAttr, GNEAttributeCarrier::AttributeValues> & 
GNEAttributeCarrier::TagValues::getAttributeValues() const {
    return myAttributeValues;
}


int 
GNEAttributeCarrier::TagValues::getPositionListed() const {
    return myPositionListed;
}


void 
GNEAttributeCarrier::TagValues::addAttribute(SumoXMLAttr attr, int attributeProperty, const std::string &definition, const std::string &defaultValue, std::vector<std::string> discreteValues) {
    if(myAttributeValues.count(attr) == 0) {
        myAttributeValues[attr] = AttributeValues(attributeProperty, (int)myAttributeValues.size(), definition, defaultValue, discreteValues);
    } else {
        throw ProcessError("Attribute '" + toString(attr) + "' already inserted");
    }
}


GUIIcon
GNEAttributeCarrier::TagValues::getGUIIcon() const {
    return myIcon;
}


SumoXMLTag 
GNEAttributeCarrier::TagValues::getParentTag() const {
    return myParentTag;
}


int
GNEAttributeCarrier::TagValues::getMaxNumberOfChilds() const {
    return myMaxNumberOfChilds;
}


bool 
GNEAttributeCarrier::TagValues::isNetElement() const {
    return (myTagProperty & TAGPROPERTY_NETELEMENT) != 0;
}


bool 
GNEAttributeCarrier::TagValues::isAdditional() const {
    return (myTagProperty & TAGPROPERTY_ADDITIONAL) != 0;
}


bool 
GNEAttributeCarrier::TagValues::isInternal() const {
    return (myTagProperty & TAGPROPERTY_INTERNAL) != 0;
}


bool 
GNEAttributeCarrier::TagValues::isShape() const {
    return (myTagProperty & TAGPROPERTY_SHAPE) != 0;
}


bool 
GNEAttributeCarrier::TagValues::isStoppingPlace() const {
    return (myTagProperty & TAGPROPERTY_STOPPINGPLACE) != 0;
}


bool 
GNEAttributeCarrier::TagValues::isDetector() const {
    return (myTagProperty & TAGPROPERTY_DETECTOR) != 0;
}


bool 
GNEAttributeCarrier::TagValues::canBlockMovement() const {
    return (myTagProperty & TAGPROPERTY_BLOCKMOVEMENT) != 0;
}


bool 
GNEAttributeCarrier::TagValues::canBlockShape() const {
    return (myTagProperty & TAGPROPERTY_BLOCKSHAPE) != 0;
}


bool 
GNEAttributeCarrier::TagValues::canCloseShape() const {
    return (myTagProperty & TAGPROPERTY_CLOSESHAPE) != 0;
}


bool 
GNEAttributeCarrier::TagValues::hasGEOPosition() const {
    return (myTagProperty & TAGPROPERTY_GEOPOSITION) != 0;
}


bool 
GNEAttributeCarrier::TagValues::hasGEOShape() const {
    return (myTagProperty & TAGPROPERTY_GEOSHAPE) != 0;
}


bool 
GNEAttributeCarrier::TagValues::hasParent() const {
    return (myTagProperty & TAGPROPERTY_PARENT) != 0;
}


bool 
GNEAttributeCarrier::TagValues::hasDialog() const {
    return (myTagProperty & TAGPROPERTY_DIALOG) != 0;
}


bool 
GNEAttributeCarrier::TagValues::hasLimitedNumberOfChilds() const {
    return (myTagProperty & TAGPROPERTY_LIMITEDCHILDS) != 0;
}


bool 
GNEAttributeCarrier::TagValues::canBeReparent() const {
    return (myTagProperty & TAGPROPERTY_REPARENT) != 0;
}

// ---------------------------------------------------------------------------
// GNEAttributeCarrier::AttributeValues - methods
// ---------------------------------------------------------------------------

GNEAttributeCarrier::AttributeValues::AttributeValues() :
    myAttributeProperty(ATTRPROPERTY_STRING),
    myPositionListed(0), 
    myDefinition(""),
    myDefaultValue("") {}


GNEAttributeCarrier::AttributeValues::AttributeValues(int attributeProperty, int positionListed, const std::string &definition, const std::string &defaultValue, const std::vector<std::string> &discreteValues) :
    myAttributeProperty(attributeProperty),
    myPositionListed(positionListed), 
    myDefinition(definition),
    myDefaultValue(defaultValue),
    myDiscreteValues(discreteValues) {
    // Check that color attributes always owns an default value
    if(isColor() && myDefaultValue.empty()) {
        throw FormatException("Color attributes must own always a default color");
    }
}


int
GNEAttributeCarrier::AttributeValues::getPositionListed() const {
    return myPositionListed;
}


const std::string&
GNEAttributeCarrier::AttributeValues::getDefinition() const {
    return myDefinition;
}


const std::string&
GNEAttributeCarrier::AttributeValues::getDefaultValue() const {
    return myDefaultValue;
}


std::string 
GNEAttributeCarrier::AttributeValues::getDescription() const {
    std::string pre;
    std::string type;
    std::string plural;
    std::string last;
    // pre type
    if((myAttributeProperty & ATTRPROPERTY_LIST) != 0) {
        pre += "list of ";
        if((myAttributeProperty & ATTRPROPERTY_VCLASS) != 0) {
            plural = "es";
        } else {
            plural = "s";
        }
    }
    if((myAttributeProperty & ATTRPROPERTY_POSITIVE) != 0) {
        pre += "positive ";
    }
    if((myAttributeProperty & ATTRPROPERTY_NONEDITABLE) != 0) {
        pre += "non editable ";
    }
    if((myAttributeProperty & ATTRPROPERTY_DISCRETE) != 0) {
        pre += "discrete ";
    }
    if((myAttributeProperty & ATTRPROPERTY_OPTIONAL) != 0) {
        pre += "optional ";
    }
    if((myAttributeProperty & ATTRPROPERTY_UNIQUE) != 0) {
        pre += "unique ";
    }
    if((myAttributeProperty & ATTRPROPERTY_COMBINABLE) != 0) {
        pre += "combinable ";
    }
    // type
    if((myAttributeProperty & ATTRPROPERTY_INT) != 0) {
        type = "integer";
    }
    if((myAttributeProperty & ATTRPROPERTY_FLOAT) != 0) {
        type = "float";
    }
    if((myAttributeProperty & ATTRPROPERTY_BOOL) != 0) {
        type = "boolean";
    }
    if((myAttributeProperty & ATTRPROPERTY_STRING) != 0) {
        type = "string";
    }
    if((myAttributeProperty & ATTRPROPERTY_POSITION) != 0) {
        type = "position";
    }
    if((myAttributeProperty & ATTRPROPERTY_COLOR) != 0) {
        type = "color";
    }
    if((myAttributeProperty & ATTRPROPERTY_VCLASS) != 0) {
        type = "VClass";
    }
    if((myAttributeProperty & ATTRPROPERTY_FILENAME) != 0) {
        type = "filename";
    }
    if((myAttributeProperty & ATTRPROPERTY_PROBABILITY) != 0) {
        type = "probability";
        last = "[0, 1]";
    }
    if((myAttributeProperty & ATTRPROPERTY_TIME) != 0) {
        type = "time";
    }
    if((myAttributeProperty & ATTRPROPERTY_ANGLE) != 0) {
        type = "angle";
        last = "[0, 360]";
    }
    return pre + type + plural + last;
}


const std::vector<std::string> &
GNEAttributeCarrier::AttributeValues::getDiscreteValues() const {
    return myDiscreteValues;
}


bool 
GNEAttributeCarrier::AttributeValues::hasDefaultValue() const {
    return (myAttributeProperty & ATTRPROPERTY_DEFAULTVALUE) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isInt() const {
    return (myAttributeProperty & ATTRPROPERTY_INT) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isFloat() const {
    return (myAttributeProperty & ATTRPROPERTY_FLOAT) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isBool() const {
    return (myAttributeProperty & ATTRPROPERTY_BOOL) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isString() const {
    return (myAttributeProperty & ATTRPROPERTY_STRING) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isProbability() const {
    return (myAttributeProperty & ATTRPROPERTY_PROBABILITY) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isNumerical() const {
    return (myAttributeProperty & (ATTRPROPERTY_INT | ATTRPROPERTY_FLOAT)) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isTime() const {
    return (myAttributeProperty & ATTRPROPERTY_TIME) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isPositive() const {
    return (myAttributeProperty & ATTRPROPERTY_POSITIVE) != 0;
}

bool 
GNEAttributeCarrier::AttributeValues::isColor() const {
    return (myAttributeProperty & ATTRPROPERTY_COLOR) != 0;
}

bool 
GNEAttributeCarrier::AttributeValues::isFilename() const {
    return (myAttributeProperty & ATTRPROPERTY_FILENAME) != 0;
}

bool 
GNEAttributeCarrier::AttributeValues::isVClass() const {
    return (myAttributeProperty & ATTRPROPERTY_VCLASS) != 0;
}

bool 
GNEAttributeCarrier::AttributeValues::isList() const {
    return (myAttributeProperty & ATTRPROPERTY_LIST) != 0;
}

bool 
GNEAttributeCarrier::AttributeValues::isUnique() const {
    return (myAttributeProperty & ATTRPROPERTY_UNIQUE) != 0;
}

bool 
GNEAttributeCarrier::AttributeValues::isOptional() const {
    return (myAttributeProperty & ATTRPROPERTY_OPTIONAL) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isDiscrete() const {
    return (myAttributeProperty & ATTRPROPERTY_DISCRETE) != 0;
}


bool 
GNEAttributeCarrier::AttributeValues::isCombinable() const {
    return (myAttributeProperty & ATTRPROPERTY_COMBINABLE) != 0;
}

// ---------------------------------------------------------------------------
// GNEAttributeCarrier - methods
// ---------------------------------------------------------------------------

GNEAttributeCarrier::GNEAttributeCarrier(SumoXMLTag tag) :
    myTag(tag),
    mySelected(false) {
}


template<> int
GNEAttributeCarrier::parse(const std::string& string) {
    return TplConvert::_str2int(string);
}


template<> double
GNEAttributeCarrier::parse(const std::string& string) {
    return TplConvert::_str2double(string);
}


template<> bool
GNEAttributeCarrier::parse(const std::string& string) {
    return TplConvert::_str2Bool(string);
}


template<> std::string
GNEAttributeCarrier::parse(const std::string& string) {
    return string;
}


template<> SUMOVehicleClass
GNEAttributeCarrier::parse(const std::string& string) {
    if (string.size() == 0) {
        throw EmptyData();
    } else if (SumoVehicleClassStrings.hasString(string) == false) {
        return SVC_IGNORING;
    } else {
        return SumoVehicleClassStrings.get(string);
    }
}


template<> RGBColor
GNEAttributeCarrier::parse(const std::string& string) {
    return RGBColor::parseColor(string);
}


template<> Position
GNEAttributeCarrier::parse(const std::string& string) {
    if (string.size() == 0) {
        throw EmptyData();
    } else {
        bool ok = true;
        PositionVector pos = GeomConvHelper::parseShapeReporting(string, "netedit-given", 0, ok, false, false);
        if(!ok || pos.size() != 1) {
            throw NumberFormatException();
        } else {
            return pos[0];
        }
    }
}


template<> SUMOVehicleShape
GNEAttributeCarrier::parse(const std::string& string) {
    if ((string == "unknown") || (SumoVehicleShapeStrings.hasString(string) == false)) {
        return SVS_UNKNOWN;
    } else {
        return SumoVehicleShapeStrings.get(string);
    }
}


template<> std::vector<std::string>
GNEAttributeCarrier::parse(const std::string& string) {
    std::vector<std::string> parsedValues;
    SUMOSAXAttributes::parseStringVector(string, parsedValues);
    return parsedValues;
}


template<> std::vector<int>
GNEAttributeCarrier::parse(const std::string& string) {
    std::vector<std::string> parsedValues = parse<std::vector<std::string> >(string);
    std::vector<int> parsedIntValues;
    for (auto i : parsedValues) {
        parsedIntValues.push_back(parse<int>(i));
    }
    return parsedIntValues;
}


template<> std::vector<double>
GNEAttributeCarrier::parse(const std::string& string) {
    std::vector<std::string> parsedValues = parse<std::vector<std::string> >(string);
    std::vector<double> parsedDoubleValues;
    for (auto i : parsedValues) {
        parsedDoubleValues.push_back(parse<double>(i));
    }
    return parsedDoubleValues;
}


template<> std::vector<bool>
GNEAttributeCarrier::parse(const std::string& string) {
    std::vector<std::string> parsedValues = parse<std::vector<std::string> >(string);
    std::vector<bool> parsedBoolValues;
    for (auto i : parsedValues) {
        parsedBoolValues.push_back(parse<bool>(i));
    }
    return parsedBoolValues;
}


template<> std::vector<GNEEdge*>
GNEAttributeCarrier::parse(GNENet* net, const std::string& value) {
    // Declare string vector
    std::vector<std::string> edgeIds = GNEAttributeCarrier::parse<std::vector<std::string> > (value);
    std::vector<GNEEdge*> parsedEdges;
    // Iterate over edges IDs, retrieve Edges and add it into parsedEdges
    for (auto i : edgeIds) {
        GNEEdge* retrievedEdge = net->retrieveEdge(i, false);
        if(retrievedEdge) {
            parsedEdges.push_back(net->retrieveEdge(i));
        } else {
            throw FormatException("Error parsing parameter " + toString(SUMO_ATTR_EDGES) + ". " + toString(SUMO_TAG_EDGE) + " '" + i + "'  doesn't exist.");
        }
    }
    return parsedEdges;
}


template<> std::vector<GNELane*>
GNEAttributeCarrier::parse(GNENet* net, const std::string& value) {
    // Declare string vector
    std::vector<std::string> laneIds = GNEAttributeCarrier::parse<std::vector<std::string> > (value);
    std::vector<GNELane*> parsedLanes;
    // Iterate over lanes IDs, retrieve Lanes and add it into parsedLanes
    for (auto i : laneIds) {
        GNELane* retrievedLane = net->retrieveLane(i, false);
        if(retrievedLane) {
            parsedLanes.push_back(net->retrieveLane(i));
        } else {
            throw FormatException("Error parsing parameter " + toString(SUMO_ATTR_LANES) + ". " + toString(SUMO_TAG_LANE) + " '" + i + "'  doesn't exist.");
        }
    }
    return parsedLanes;
}


template<> std::string
GNEAttributeCarrier::parseIDs(const std::vector<GNEEdge*> &ACs) {
    // obtain ID's of edges and return their join
    std::vector<std::string> edgeIDs;
    for (auto i : ACs) {
        edgeIDs.push_back(i->getID());
    }
    return joinToString(edgeIDs, " ");
}


template<> std::string
GNEAttributeCarrier::parseIDs(const std::vector<GNELane*> &ACs) {
    // obtain ID's of lanes and return their join
    std::vector<std::string> laneIDs;
    for (auto i : ACs) {
        laneIDs.push_back(i->getID());
    }
    return joinToString(laneIDs, " ");
}


bool 
GNEAttributeCarrier::parseStringToANDBool(const std::string& string) {
    // obtain boolean vector (throw exception if string is empty)
    std::vector<bool> boolValues = GNEAttributeCarrier::parse<std::vector<bool> >(string);
    // set value of checkbox
    if (boolValues.size() == 1) {
        return boolValues.front();
    } else {
        int sum = 0;
        for (auto i : boolValues) {
            sum += (int)(i);
        }
        // only return true if all values are true
        if ((sum == 0) || (sum != (int)boolValues.size())) {
            return false;
        } else {
            return true;
        }
    }
}


std::string
GNEAttributeCarrier::getAttributeForSelection(SumoXMLAttr key) const {
    return getAttribute(key);
}


SumoXMLTag
GNEAttributeCarrier::getTag() const {
    return myTag;
}


FXIcon* 
GNEAttributeCarrier::getIcon() const {
    // define on first access
    if (myAllowedAttributes.size() == 0) {
        fillAttributeCarriers();
    }
    return GUIIconSubSys::getIcon(myAllowedAttributes.at(myTag).getGUIIcon());
}


const std::string
GNEAttributeCarrier::getID() const {
    return getAttribute(SUMO_ATTR_ID);
}


bool
GNEAttributeCarrier::isValidID(const std::string& value) {
    if(value.size() == 0) {
        return false;
    } else {
        return value.find_first_of(" \t\n\r@$%^&/|\\{}*'\";:<>") == std::string::npos;
    }
}


bool
GNEAttributeCarrier::isValidFilename(const std::string& value) {
    // @note Only characteres that aren't permited in a file path or belong
    // to XML sintax
    return (value.find_first_of("\t\n\r@$%^&|\\{}*'\";:<>") == std::string::npos);
}

// ===========================================================================
// static methods
// ===========================================================================

const GNEAttributeCarrier::TagValues & 
GNEAttributeCarrier::getTagProperties(SumoXMLTag tag) {
    // define on first access
    if (myAllowedAttributes.size() == 0) {
        fillAttributeCarriers();
    }
    // check that tag is defined
    if(myAllowedAttributes.count(tag) == 0) {
        throw ProcessError("Attributes for tag '" + toString(tag) + "' not defined");
    } else {
        return myAllowedAttributes.at(tag);
    }
}


const GNEAttributeCarrier::AttributeValues &
GNEAttributeCarrier::getAttributeProperties(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myAllowedAttributes.size() == 0) {
        fillAttributeCarriers();
    }
    // check that tag is defined
    if(myAllowedAttributes.count(tag) == 0) {
        throw ProcessError("Attributes for tag '" + toString(tag) + "' not defined");
    } else if(myAllowedAttributes.at(tag).getAttributeValues().count(attr) == 0) {
        throw ProcessError("Attributes for tag '" + toString(tag) + "' and attribute '" + toString(attr) + "' not defined");
    } else {
        return myAllowedAttributes.at(tag).getAttributeValues().at(attr);
    }
}


std::vector<SumoXMLTag>
GNEAttributeCarrier::allowedTags(bool includingInternals) {
    std::vector<SumoXMLTag> allTags;
    // define on first access
    if (myAllowedAttributes.size() == 0) {
        fillAttributeCarriers();
    }
    // fill all tags
    for (auto i : myAllowedAttributes) {
        if (includingInternals || !i.second.isInternal()) {
            allTags.push_back(i.first);
        }
    }
    return allTags;
}


std::vector<SumoXMLTag>
GNEAttributeCarrier::allowedNetElementsTags(bool includingInternals) {
    std::vector<SumoXMLTag> netElementTags;
    // define on first access
    if (myAllowedAttributes.size() == 0) {
        fillAttributeCarriers();
    }
    // fill netElements tags
    for (auto i : myAllowedAttributes) {
        if (i.second.isNetElement() && (includingInternals || !i.second.isInternal())) {
            netElementTags.push_back(i.first);
        }
    }
    return netElementTags;
}


std::vector<SumoXMLTag>
GNEAttributeCarrier::allowedAdditionalTags(bool includingInternals) {
    std::vector<SumoXMLTag> additionalTags;
    // define on first access
    if (myAllowedAttributes.size() == 0) {
        fillAttributeCarriers();
    }
    // fill additional tags
    for (auto i : myAllowedAttributes) {
        if (i.second.isAdditional() && (includingInternals || !i.second.isInternal())) {
            additionalTags.push_back(i.first);
        }
    }
    return additionalTags;
}


std::vector<SumoXMLTag>
GNEAttributeCarrier::allowedShapeTags(bool includingInternals) {
    std::vector<SumoXMLTag> shapeTags;
    // define on first access
    if (myAllowedAttributes.size() == 0) {
        fillAttributeCarriers();
    }
    // fill shape tags
    for (auto i : myAllowedAttributes) {
        if (i.second.isShape() && (includingInternals || !i.second.isInternal())) {
            shapeTags.push_back(i.first);
        }
    }
    return shapeTags;
}


bool
GNEAttributeCarrier::hasAttribute(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myAllowedAttributes.size() == 0) {
        fillAttributeCarriers();
    }
    if(myAllowedAttributes.count(tag) == 0) {
        throw ProcessError("Attributes for tag '" + toString(tag) + "' not defined");
    } else {
        return (myAllowedAttributes.at(tag).getAttributeValues().count(attr) != 0);
    }
}


int
GNEAttributeCarrier::getHigherNumberOfAttributes() {
    int maxNumAttribute = 0;
    // define on first access
    if (myAllowedAttributes.size() == 0) {
        fillAttributeCarriers();
    }
    // get max num attributes
    for (auto i : myAllowedAttributes) {
        maxNumAttribute = MAX2(maxNumAttribute, (int)i.second.getAttributeValues().size());
    }
    return maxNumAttribute;
}


const std::string&
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    const AttributeValues & attrValue = getAttributeProperties(tag, attr);
    if(attrValue.hasDefaultValue()) {
        return attrValue.getDefaultValue();
    } else {
        // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
        throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
    }
}


int 
GNEAttributeCarrier::getCircleResolution(const GUIVisualizationSettings& settings) {
    if(settings.drawForSelecting) {
        return 8;
    } else if (settings.scale >= 10) {
        return 32;
    } else if (settings.scale >= 2) {
        return 16;
    } else {
        return 8;
    }
}


void 
GNEAttributeCarrier::writeAttribute(OutputDevice& device, SumoXMLAttr key) const {
    // obtain attribute property (only for improve code legibility)
    const GNEAttributeCarrier::AttributeValues &attrValue = GNEAttributeCarrier::getAttributeProperties(getTag(), key);
    std::string attribute = getAttribute(key);
    if(attrValue.isOptional()) {
        // only write optional attributes (i.e attributes with default value) if are differents
        if(attrValue.getDefaultValue() != attribute) {
            device.writeAttr(key, attribute);
        }
    } else {
        device.writeAttr(key, attribute);
    }
}


void 
GNEAttributeCarrier::fillAttributeCarriers() {
    const OptionsCont& oc = OptionsCont::getOptions();
    // obtain Node Types except NODETYPE_DEAD_END_DEPRECATED
    std::vector<std::string> nodeTypes = SUMOXMLDefinitions::NodeTypes.getStrings();
    nodeTypes.erase(std::find(nodeTypes.begin(), nodeTypes.end(), toString(NODETYPE_DEAD_END_DEPRECATED)));
    // fill all ACs
    SumoXMLTag currentTag = SUMO_TAG_EDGE;
    {
        // position in attribute list
        int pos = 0;
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_NETELEMENT, 1, ICON_EDGE);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the edge", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FROM,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The name of a node within the nodes-file the edge shall start at", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TO,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The name of a node within the nodes-file the edge shall end at", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SPEED,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "The maximum speed allowed on the edge in m/s", 
            toString(oc.getFloat("default.speed")));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PRIORITY,
            ATTRPROPERTY_INT | ATTRPROPERTY_DEFAULTVALUE,
            "The priority of the edge", 
            toString(oc.getInt("default.priority")));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_NUMLANES,
            ATTRPROPERTY_INT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "The number of lanes of the edge", 
            toString(oc.getInt("default.lanenumber")));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TYPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE,
            "The name of a type within the SUMO edge type file", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ALLOW,
            ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DISCRETE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_COMBINABLE,
            "Explicitly allows the given vehicle classes (not given will be not allowed)", 
            "all",
            SumoVehicleClassStrings.getStrings());
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_DISALLOW,
            ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DISCRETE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_COMBINABLE,
            "Explicitly disallows the given vehicle classes (not given will be allowed)", 
            "",
            SumoVehicleClassStrings.getStrings());
        //myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PREFER, );
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SHAPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_LIST | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE,
            "If the shape is given it should start and end with the positions of the from-node and to-node", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LENGTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE,
            "The length of the edge in meter", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SPREADTYPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DISCRETE | ATTRPROPERTY_DEFAULTVALUE,
            "Lane width for all lanes of this edge in meters (used for visualization)", 
            "right",
            SUMOXMLDefinitions::LaneSpreadFunctions.getStrings());
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_NAME,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE,
            "street name (need not be unique, used for visualization)", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_WIDTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "Lane width for all lanes of this edge in meters (used for visualization)", 
            "default");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ENDOFFSET,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "Move the stop line back from the intersection by the given amount", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(GNE_ATTR_SHAPE_START,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_DEFAULTVALUE, // virtual attribute used to define an endPoint
            "Custom position in which shape start (by default position of junction from)", 
            "");
        myAllowedAttributes[currentTag].addAttribute(GNE_ATTR_SHAPE_END,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_DEFAULTVALUE, // virtual attribute from to define an endPoint
            "Custom position in which shape end (by default position of junction from)", 
            "");
        myAllowedAttributes[currentTag].addAttribute(GNE_ATTR_BIDIR,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE, // virtual attribute to check of this edge is part of a bidirectional railway
            "Enable or disable bidireccional edge", 
            "false");
    }
    currentTag = SUMO_TAG_JUNCTION;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_NETELEMENT, 2, ICON_JUNCTION);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the node", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION, // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
            "The x-y-z position of the node on the plane in meters", 
            ""); 
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TYPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DISCRETE | ATTRPROPERTY_DEFAULTVALUE,
            "An optional type for the node", 
            "",
            nodeTypes);
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SHAPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_LIST | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE,
            "A custom shape for that node", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_RADIUS,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE,
            "Optional turning radius (for all corners) for that node in meters", 
            "1.5");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_KEEP_CLEAR,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE,
            "Whether the junction-blocking-heuristic should be activated at this node", 
            "1");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TLTYPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DISCRETE | ATTRPROPERTY_DEFAULTVALUE,
            "An optional type for the traffic light algorithm", 
            "",
            SUMOXMLDefinitions::TrafficLightTypes.getStrings());
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TLID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE,
            "An optional id for the traffic light program", 
            "");
    }
    currentTag = SUMO_TAG_LANE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_NETELEMENT, 3, ICON_LANE);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "ID of lane (Automatic)", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SPEED,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "Speed in meters per second", 
            "13.89");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ALLOW,
            ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DISCRETE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_COMBINABLE,
            "Explicitly allows the given vehicle classes (not given will be not allowed)", 
            "all",
            SumoVehicleClassStrings.getStrings());
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_DISALLOW,
            ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DISCRETE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_COMBINABLE,
            "Explicitly disallows the given vehicle classes (not given will be allowed)", 
            "",
            SumoVehicleClassStrings.getStrings());
        //myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PREFER, );
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_WIDTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "Width in meters (used for visualization)", 
            "default");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ENDOFFSET,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "Move the stop line back from the intersection by the given amount",
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ACCELERATION,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE,
            "Enable or disable lane as acceleration lane", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CUSTOMSHAPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE,
            "If the shape is given it overrides the computation based on edge shape", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_INDEX,
            ATTRPROPERTY_INT | ATTRPROPERTY_NONEDITABLE,
            "The enumeration index of the lane (0 is the rightmost lane, <NUMBER_LANES>-1 is the leftmost one)", 
            "");
    }
    currentTag = SUMO_TAG_CROSSING;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_NETELEMENT, 4, ICON_CROSSING);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_NONEDITABLE,
            "The ID of Crossing", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_EDGES,
            ATTRPROPERTY_STRING | ATTRPROPERTY_LIST | ATTRPROPERTY_UNIQUE,
            "The (road) edges which are crossed", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PRIORITY,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE,
            "Whether the pedestrians have priority over the vehicles (automatically set to true at tls-controlled intersections)", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_WIDTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "The width of the crossings", 
            toString(OptionsCont::getOptions().getFloat("default.crossing-width")));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TLLINKINDEX,
            ATTRPROPERTY_INT | ATTRPROPERTY_DEFAULTVALUE,
            "sets the tls-index for this crossing", 
            "-1");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TLLINKINDEX2,
            ATTRPROPERTY_INT | ATTRPROPERTY_DEFAULTVALUE,
            "sets the opposite-direction tls-index for this crossing", 
            "-1");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CUSTOMSHAPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_LIST | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE,
            "Overrids default shape of pedestrian crossing", 
            "");
    }
    currentTag = SUMO_TAG_CONNECTION;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_NETELEMENT, 5, ICON_CONNECTION);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FROM,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_NONEDITABLE,
            "The name of the edge the vehicles leave", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TO,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_NONEDITABLE,
            "The name of the edge the vehicles may reach when leaving 'from'", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FROM_LANE,
            ATTRPROPERTY_INT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_NONEDITABLE,
            "the lane index of the incoming lane (numbers starting with 0)", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TO_LANE,
            ATTRPROPERTY_INT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_NONEDITABLE,
            "the lane index of the outgoing lane (numbers starting with 0)", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PASS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE,
            "if set, vehicles which pass this (lane-2-lane) connection) will not wait", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_KEEP_CLEAR,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE,
            "if set to false, vehicles which pass this (lane-2-lane) connection) will not worry about blocking the intersection", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CONTPOS,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "If set to a more than 0 value, an internal junction will be built at this position (in m) from the start of the internal lane for this connection", 
            toString(NBEdge::UNSPECIFIED_CONTPOS));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_UNCONTROLLED,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE,
            "If set to true, This connection will not be TLS-controlled despite its node being controlled", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_VISIBILITY_DISTANCE,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "", 
            toString(NBEdge::UNSPECIFIED_VISIBILITY_DISTANCE));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TLLINKINDEX,
            ATTRPROPERTY_INT | ATTRPROPERTY_DEFAULTVALUE,
            "sets the distance to the connection at which all relevant foes are visible", 
            "-1");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SPEED,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "sets custom speed limit for the connection", 
            toString(NBEdge::UNSPECIFIED_SPEED));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CUSTOMSHAPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_LIST | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE,
            "sets custom shape for the connection", 
            "");
    }
    currentTag = SUMO_TAG_BUS_STOP;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_STOPPINGPLACE | TAGPROPERTY_BLOCKMOVEMENT, 10, ICON_BUSSTOP, SUMO_TAG_NOTHING);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of bus stop", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The name of the lane the bus stop shall be located at", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_STARTPOS,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The begin position on the lane (the lower position on the lane) in meters", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ENDPOS,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The end position on the lane (the higher position on the lane) in meters, must be larger than startPos by more than 0.1m", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_NAME,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Name of bus stop", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LINES,
            ATTRPROPERTY_STRING | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Meant to be the names of the bus lines that stop at this bus stop. This is only used for visualization purposes", 
            "");
    }
    currentTag = SUMO_TAG_ACCESS;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_PARENT | TAGPROPERTY_BLOCKMOVEMENT, 11, ICON_BUSSTOP /* temporal */, SUMO_TAG_BUS_STOP);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The name of the lane the stop access shall be located at", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The position on the lane (the lower position on the lane) in meters", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LENGTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The walking length of the access in meters", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
    }
    currentTag = SUMO_TAG_CONTAINER_STOP;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_STOPPINGPLACE | TAGPROPERTY_BLOCKMOVEMENT, 12, ICON_CONTAINERSTOP);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of container stop", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The name of the lane the container stop shall be located at", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_STARTPOS,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The begin position on the lane (the lower position on the lane) in meters", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ENDPOS,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The end position on the lane (the higher position on the lane) in meters, must be larger than startPos by more than 0.1m", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_NAME,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Name of Container Stop", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LINES,
            ATTRPROPERTY_STRING | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "meant to be the names of the bus lines that stop at this container stop. This is only used for visualization purposes", 
            "");
    }
    currentTag = SUMO_TAG_CHARGING_STATION;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_STOPPINGPLACE | TAGPROPERTY_BLOCKMOVEMENT, 13, ICON_CHARGINGSTATION);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of charging station", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "Lane of the charging station location", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_STARTPOS,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Begin position in the specified lane", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ENDPOS,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "End position in the specified lane", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_NAME,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Name of Charging Station", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CHARGINGPOWER,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Charging power in W", 
            "22000.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_EFFICIENCY,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_PROBABILITY | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Charging efficiency [0,1]", 
            "0.95");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CHARGEINTRANSIT,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Enable or disable charge in transit, i.e. vehicle must or must not to stop for charging", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CHARGEDELAY,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Time delay after the vehicles has reached / stopped on the charging station, before the energy transfer (charging) begins", 
            "0.00");
    }
    currentTag = SUMO_TAG_PARKING_AREA;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_STOPPINGPLACE | TAGPROPERTY_BLOCKMOVEMENT, 14, ICON_PARKINGAREA);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of ParkingArea", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The name of the lane the Parking Area shall be located at", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_STARTPOS,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The begin position on the lane (the lower position on the lane) in meters", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ENDPOS,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The end position on the lane (the higher position on the lane) in meters, must be larger than startPos by more than 0.1m", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ROADSIDE_CAPACITY,
            ATTRPROPERTY_INT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            " The number of parking spaces for road-side parking ", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_NAME,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Name of Parking Area", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_WIDTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The width of the road-side parking spaces", 
            "3.20");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LENGTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_OPTIONAL | ATTRPROPERTY_DEFAULTVALUE,
            "The length of the road-side parking spaces. By default (endPos - startPos) / roadsideCapacity", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ANGLE,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The angle of the road-side parking spaces relative to the lane angle, positive means clockwise", 
            "0.00");
    }
    currentTag = SUMO_TAG_PARKING_SPACE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_PARENT | TAGPROPERTY_REPARENT | TAGPROPERTY_BLOCKMOVEMENT, 15, ICON_PARKINGSPACE, SUMO_TAG_PARKING_AREA);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_X,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE,
            "The X position in meters of the parking vehicle", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_Y,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE,
            "The Y position in meters of the parking vehicle", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_Z,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The Z position in meters of the parking vehicle", 
            "0.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_WIDTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The width of the road-side parking spaces", 
            "3.2");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LENGTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The length of the road-side parking spaces", 
            "5.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ANGLE,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_ANGLE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The angle of the road-side parking spaces relative to the lane angle, positive means clockwise", 
            "0");
    }
    currentTag = SUMO_TAG_E1DETECTOR;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_DETECTOR | TAGPROPERTY_BLOCKMOVEMENT, 20, ICON_E1);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of E1", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the lane the detector shall be laid on. The lane must be a part of the network used", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE,
            "The position on the lane the detector shall be laid on in meters. The position must be a value between -1*lane's length and the lane's length", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FREQUENCY,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "The aggregation period the values the detector collects shall be summed up", 
            "900.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE,
            "The path to the output file", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_VTYPES,
            ATTRPROPERTY_STRING | ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE,
            "Space separated list of vehicle type ids to consider", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
    }
    currentTag = SUMO_TAG_E2DETECTOR;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_DETECTOR | TAGPROPERTY_BLOCKMOVEMENT, 21, ICON_E2);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of E2", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the lane the detector shall be laid on. The lane must be a part of the network used", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE,
            "The position on the lane the detector shall be laid on in meters", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LENGTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "The length of the detector in meters", 
            "10.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FREQUENCY,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "The aggregation period the values the detector collects shall be summed up", 
            "900.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The path to the output file", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CONT,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Holds the information whether detectors longer than a lane shall be cut off or continued (set it to true for the second case))", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_HALTING_TIME_THRESHOLD,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The time-based threshold that describes how much time has to pass until a vehicle is recognized as halting)", 
            "1.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_HALTING_SPEED_THRESHOLD,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The speed-based threshold that describes how slow a vehicle has to be to be recognized as halting) in m/s", 
            "1.39");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_JAM_DIST_THRESHOLD,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The minimum distance to the next standing vehicle in order to make this vehicle count as a participant to the jam) in m", 
            "10.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
    }
    currentTag = SUMO_TAG_E3DETECTOR;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_DETECTOR | TAGPROPERTY_BLOCKMOVEMENT, 22, ICON_E3);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of E3", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE,
            "X-Y position of detector in editor (Only used in NETEDIT)", 
            "0,0"); // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FREQUENCY,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "The aggregation period the values the detector collects shall be summed up", 
            "900.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The path to the output file", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_HALTING_TIME_THRESHOLD,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The time-based threshold that describes how much time has to pass until a vehicle is recognized as halting) in s", 
            "1.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_HALTING_SPEED_THRESHOLD,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The speed-based threshold that describes how slow a vehicle has to be to be recognized as halting) in m/s", 
            "1.39");
    }
    currentTag = SUMO_TAG_DET_ENTRY;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_DETECTOR | TAGPROPERTY_PARENT | TAGPROPERTY_REPARENT | TAGPROPERTY_BLOCKMOVEMENT, 23, ICON_E3ENTRY, SUMO_TAG_E3DETECTOR);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the lane the detector shall be laid on. The lane must be a part of the network used", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE,
            "The position on the lane the detector shall be laid on in meters", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
    }
    currentTag = SUMO_TAG_DET_EXIT;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_DETECTOR | TAGPROPERTY_PARENT | TAGPROPERTY_REPARENT | TAGPROPERTY_BLOCKMOVEMENT, 24, ICON_E3EXIT, SUMO_TAG_E3DETECTOR);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the lane the detector shall be laid on. The lane must be a part of the network used", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE,
            "The position on the lane the detector shall be laid on in meters", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
    }
    currentTag = SUMO_TAG_INSTANT_INDUCTION_LOOP;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_DETECTOR | TAGPROPERTY_BLOCKMOVEMENT, 25, ICON_E1INSTANT);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of Instant Induction Loop (E1Instant)", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the lane the detector shall be laid on. The lane must be a part of the network used", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE,
            "The position on the lane the detector shall be laid on in meters. The position must be a value between -1*lane's length and the lane's length", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FREQUENCY,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "The aggregation period the values the detector collects shall be summed up", 
            "900.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE,
            "The path to the output file", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_VTYPES,
            ATTRPROPERTY_STRING | ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE,
            "Space separated list of vehicle type ids to consider", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FRIENDLY_POS,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1", 
            "0");
    }
    currentTag = SUMO_TAG_VSS;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_BLOCKMOVEMENT | TAGPROPERTY_DIALOG, 30, ICON_VARIABLESPEEDSIGN);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE,
            "The id of Variable Speed Signal", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE,
            "X-Y position of detector in editor (Only used in NETEDIT)", 
            "0,0"); // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANES,
            ATTRPROPERTY_STRING | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE,
            "list of lanes of Variable Speed Sign", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The path to the output file", 
            "");
    }
    currentTag = SUMO_TAG_STEP;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_INTERNAL, 31, ICON_STEP);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TIME,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME,
            "Time", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SPEED,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_DEFAULTVALUE,
            "Speed", 
            "13.89");
    }
    currentTag = SUMO_TAG_CALIBRATOR;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_DIALOG, 40, ICON_CALIBRATOR);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of Calibrator", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_EDGE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of edge in the simulation network", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "The position of the calibrator on the specified lane", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FREQUENCY,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The aggregation interval in which to calibrate the flows. default is step-length", 
            "1.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ROUTEPROBE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The id of the routeProbe element from which to determine the route distribution for generated vehicles", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_OUTPUT,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The output file for writing calibrator information or NULL", 
            "");
    }
    currentTag = SUMO_TAG_LANECALIBRATOR;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_DIALOG, 41, ICON_CALIBRATOR);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of Calibrator", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of lane in the simulation network", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "The position of the calibrator on the specified lane", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FREQUENCY,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The aggregation interval in which to calibrate the flows. default is step-length", 
            "100.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ROUTEPROBE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The id of the routeProbe element from which to determine the route distribution for generated vehicles", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_OUTPUT,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The output file for writing calibrator information or NULL",
            "");
    }
    currentTag = SUMO_TAG_FLOW;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_PARENT | TAGPROPERTY_INTERNAL, 42, ICON_EMPTY /* temporal */, SUMO_TAG_CALIBRATOR);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the vehicle type to use for this vehicle", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TYPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE,
            "The id of the vehicle type to use for this vehicle", 
            DEFAULT_VTYPE_ID);
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ROUTE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE,
            "The id of the route the vehicle shall drive along", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_VEHSPERHOUR,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Number of vehicles per hour, equally spaced", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SPEED,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Speed of vehicles", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_COLOR,
            ATTRPROPERTY_STRING | ATTRPROPERTY_COLOR | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "This vehicle's color", 
            "yellow");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_BEGIN,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "First vehicle departure time",  
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_END,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "End of departure interval", 
            "3600.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_DEPARTLANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The lane on which the vehicle shall be inserted", 
            "first");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_DEPARTPOS,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL, 
            "The position at which the vehicle shall enter the net", 
            "base");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_DEPARTSPEED,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The speed with which the vehicle shall enter the network", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ARRIVALLANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The lane at which the vehicle shall leave the network", 
            "current");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ARRIVALPOS,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The position at which the vehicle shall leave the network", 
            "max");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ARRIVALSPEED,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The speed with which the vehicle shall leave the network", 
            "current");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LINE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "A string specifying the id of a public transport line which can be used when specifying person rides", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PERSON_NUMBER,
            ATTRPROPERTY_INT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The number of occupied seats when the vehicle is inserted", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CONTAINER_NUMBER,
            ATTRPROPERTY_INT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The number of occupied container places when the vehicle is inserted",  
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_REROUTE,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Whether the vehicle should be equipped with a rerouting device", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_DEPARTPOS_LAT,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The lateral position on the departure lane at which the vehicle shall enter the net", 
            "center");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ARRIVALPOS_LAT,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The lateral position on the arrival lane at which the vehicle shall arrive", 
            "");
    }
    currentTag = SUMO_TAG_ROUTE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_PARENT | TAGPROPERTY_INTERNAL, 43, ICON_EMPTY /* temporal */, SUMO_TAG_CALIBRATOR);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of Route", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_EDGES,
            ATTRPROPERTY_STRING | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE,
            "The edges the vehicle shall drive along, given as their ids, separated using spaces", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_COLOR,
            ATTRPROPERTY_STRING | ATTRPROPERTY_COLOR | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "This route's color", 
            "yellow");
    }
    currentTag = SUMO_TAG_REROUTER;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_BLOCKMOVEMENT | TAGPROPERTY_DIALOG, 50, ICON_REROUTER);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of Rerouter", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_EDGES,
            ATTRPROPERTY_STRING | ATTRPROPERTY_LIST | ATTRPROPERTY_UNIQUE,
            "An edge id or a list of edge ids where vehicles shall be rerouted", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_UNIQUE | ATTRPROPERTY_DEFAULTVALUE,
            "X,Y position in editor (Only used in NETEDIT)", 
            "0,0"); // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The path to the definition file (alternatively, the intervals may defined as children of the rerouter)", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PROB,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_PROBABILITY | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The probability for vehicle rerouting (0-1)", 
            "1.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_HALTING_TIME_THRESHOLD,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The waiting time threshold (in s) that must be reached to activate rerouting (default -1 which disables the threshold)", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_OFF,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Whether the router should be inactive initially (and switched on in the gui)", 
            "0");
    }
    currentTag = SUMO_TAG_INTERVAL;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_INTERNAL | TAGPROPERTY_PARENT, 51, ICON_EMPTY, SUMO_TAG_REROUTER);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_BEGIN,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "Begin", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_END,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "End", 
            "3600.00");
    }
    currentTag = SUMO_TAG_CLOSING_REROUTE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_INTERNAL | TAGPROPERTY_PARENT, 52, ICON_REROUTER, SUMO_TAG_INTERVAL);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "Edge ID", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ALLOW,
            ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_COMBINABLE | ATTRPROPERTY_OPTIONAL,
            "allowed vehicles", 
            "all"); 
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_DISALLOW,
            ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_COMBINABLE | ATTRPROPERTY_OPTIONAL,
            "disallowed vehicles", 
            "");
    }
    currentTag = SUMO_TAG_CLOSING_LANE_REROUTE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_INTERNAL | TAGPROPERTY_PARENT, 53, ICON_REROUTER, SUMO_TAG_INTERVAL);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "Lane ID", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ALLOW,
            ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_COMBINABLE | ATTRPROPERTY_OPTIONAL,
            "allowed vehicles", 
            "all");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_DISALLOW,
            ATTRPROPERTY_VCLASS | ATTRPROPERTY_LIST | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_COMBINABLE | ATTRPROPERTY_OPTIONAL,
            "disallowed vehicles", 
            "");
    }
    currentTag = SUMO_TAG_DEST_PROB_REROUTE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_INTERNAL | TAGPROPERTY_PARENT, 54, ICON_EMPTY, SUMO_TAG_INTERVAL);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "Edge ID", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PROB,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_PROBABILITY | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "probability", 
            "1.00");
    }
    currentTag = SUMO_TAG_PARKING_ZONE_REROUTE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_INTERNAL | TAGPROPERTY_PARENT, 55, ICON_EMPTY, SUMO_TAG_INTERVAL);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "ParkingArea ID", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PROB,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_PROBABILITY | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "probability", 
            "1.00");
    }
    currentTag = SUMO_TAG_ROUTE_PROB_REROUTE; 
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_INTERNAL, 56, ICON_EMPTY, SUMO_TAG_INTERVAL);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "Route", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PROB,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_PROBABILITY | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "probability", 
            "1.00");
    }
    currentTag = SUMO_TAG_ROUTEPROBE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL, 60, ICON_ROUTEPROBE);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of RouteProbe", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_EDGE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of an edge in the simulation network", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FREQUENCY,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE,
            "The frequency in which to report the distribution", 
            "3600");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE,
            "The file for generated output", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_BEGIN,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The time at which to start generating output", 
            "0");
    }
    currentTag = SUMO_TAG_VAPORIZER;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL, 70, ICON_VAPORIZER);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "Edge in which vaporizer is placed", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_BEGIN,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "Start Time", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_END,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE,
            "End Time", 
            "3600.00");
    }
    currentTag = SUMO_TAG_VTYPE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_ADDITIONAL | TAGPROPERTY_PARENT | TAGPROPERTY_INTERNAL, 80, ICON_EMPTY /* temporal */);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of VehicleType", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ACCEL,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The acceleration ability of vehicles of this type [m/s^2]", 
            "2.60");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_DECEL,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The deceleration ability of vehicles of this type [m/s^2]", 
            "4.50");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SIGMA,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Car-following model parameter", 
            "0.50");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TAU,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Car-following model parameter", 
            "1.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LENGTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The vehicle's netto-length (length) [m]", 
            "5.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_MINGAP,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Empty space after leader [m]", 
            "2.50");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_MAXSPEED,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The vehicle's maximum velocity [m/s]", 
            "70.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SPEEDFACTOR,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The vehicles expected multiplicator for lane speed limits", 
            "1.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SPEEDDEV,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The deviation of the speedFactor", 
            "0.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_COLOR,
            ATTRPROPERTY_STRING | ATTRPROPERTY_COLOR | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "This vehicle type's color", 
            "1,1,0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_VCLASS,
            ATTRPROPERTY_VCLASS | ATTRPROPERTY_DISCRETE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "An abstract vehicle class", 
            "passenger",
            SumoVehicleShapeStrings.getStrings());
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_EMISSIONCLASS,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "An abstract emission class", 
            "P_7_7");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_GUISHAPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DISCRETE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "How this vehicle is rendered", 
            "",
            SumoVehicleShapeStrings.getStrings());
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_WIDTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The vehicle's width [m] (only used for drawing)", 
            "2.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_IMGFILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Image file for rendering vehicles of this type (should be grayscale to allow functional coloring)", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_IMPATIENCE,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Willingess of drivers to impede vehicles with higher priority", 
            "0.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE_CHANGE_MODEL,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The model used for changing lanes", 
            "LC2013");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CAR_FOLLOW_MODEL,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The model used for car following", 
            "Krauss");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_PERSON_CAPACITY,
            ATTRPROPERTY_INT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE| ATTRPROPERTY_OPTIONAL,
            "The number of persons (excluding an autonomous driver) the vehicle can transport", 
            "4");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_CONTAINER_CAPACITY,
            ATTRPROPERTY_INT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE| ATTRPROPERTY_OPTIONAL,
            "The number of containers the vehicle can transport", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_BOARDING_DURATION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME | ATTRPROPERTY_DEFAULTVALUE| ATTRPROPERTY_OPTIONAL,
            "The time required by a person to board the vehicle", 
            "0.50");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LOADING_DURATION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_TIME| ATTRPROPERTY_DEFAULTVALUE| ATTRPROPERTY_OPTIONAL,
            "The time required to load a container onto the vehicle", 
            "90.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LATALIGNMENT,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE,
            "The preferred lateral alignment when using the sublane-model", 
            "center");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_MINGAP_LAT,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "The minimum lateral gap at a speed difference of 50km/h when using the sublane-model", 
            "0.12");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_MAXSPEED_LAT,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE,
            "The maximum lateral speed when using the sublane-model", 
            "1.00");
    }
    currentTag = SUMO_TAG_POLY;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_SHAPE | TAGPROPERTY_BLOCKMOVEMENT | TAGPROPERTY_BLOCKSHAPE | TAGPROPERTY_CLOSESHAPE | TAGPROPERTY_GEOSHAPE, 100, ICON_LOCATEPOLY /* temporal */);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the polygon", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_SHAPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_LIST | ATTRPROPERTY_UNIQUE,
            "The shape of the polygon", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_COLOR,
            ATTRPROPERTY_STRING | ATTRPROPERTY_COLOR | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The RGBA color with which the polygon shall be displayed", 
            "red");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_FILL,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE,
            "An information whether the polygon shall be filled", 
            "0");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LAYER,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE,
            "The layer in which the polygon lies", 
            toString(Shape::DEFAULT_LAYER));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TYPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE| ATTRPROPERTY_OPTIONAL,
            "A typename for the polygon", 
            toString(Shape::DEFAULT_TYPE));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_IMGFILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE| ATTRPROPERTY_OPTIONAL,
            "A bitmap to use for rendering this polygon", 
            toString(Shape::DEFAULT_IMG_FILE));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_RELATIVEPATH,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Enable or disbale use image file as a relative path", 
            toString(Shape::DEFAULT_RELATIVEPATH));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ANGLE,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_ANGLE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Angle of rendered image in degree", 
            toString(Shape::DEFAULT_ANGLE));
    }
    currentTag = SUMO_TAG_POI;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_SHAPE | TAGPROPERTY_BLOCKMOVEMENT | TAGPROPERTY_GEOPOSITION, 110, ICON_LOCATEPOI /* temporal */);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the POI", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_STRING | ATTRPROPERTY_POSITION | ATTRPROPERTY_UNIQUE, // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
            "The position in view", 
            ""); 
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_COLOR,
            ATTRPROPERTY_STRING | ATTRPROPERTY_COLOR | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The color with which the poi shall be displayed", 
            "red");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TYPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "A typename for the poi", 
            toString(Shape::DEFAULT_TYPE));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LAYER,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE,
            "The layer of the poi for drawing and selecting", 
            toString(Shape::DEFAULT_LAYER_POI));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_WIDTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Width of rendered image in meters", 
            toString(Shape::DEFAULT_IMG_WIDTH));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_HEIGHT,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Height of rendered image in meters", 
            toString(Shape::DEFAULT_IMG_HEIGHT));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_IMGFILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "A bitmap to use for rendering this poi", 
            toString(Shape::DEFAULT_IMG_FILE));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_RELATIVEPATH,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Enable or disable use image file as a relative path", 
            toString(Shape::DEFAULT_RELATIVEPATH));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ANGLE,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_ANGLE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Angle of rendered image in degree", 
            toString(Shape::DEFAULT_ANGLE));
    }
    currentTag = SUMO_TAG_POILANE;
    {
        // set values of tag
        myAllowedAttributes[currentTag] = TagValues(TAGPROPERTY_SHAPE | TAGPROPERTY_BLOCKMOVEMENT, 111, ICON_LOCATEPOI /* temporal */);
        // set values of attributes
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ID,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The id of the POI", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LANE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_UNIQUE,
            "The name of the lane the poi is located at", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_UNIQUE,
            "The position on the named lane or in the net in meters at which the poi is located at", 
            "");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_POSITION_LAT,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The lateral offset on the named lane at which the poi is located at", 
            "0.00");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_COLOR,
            ATTRPROPERTY_STRING | ATTRPROPERTY_COLOR | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "The color with which the poi shall be displayed", 
            "red");
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_TYPE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "A typename for the poi", 
            toString(Shape::DEFAULT_TYPE));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_LAYER,
            ATTRPROPERTY_STRING | ATTRPROPERTY_DEFAULTVALUE,
            "The layer of the poi for drawing and selecting", 
            toString(Shape::DEFAULT_LAYER_POI));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_WIDTH,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Width of rendered image in meters", 
            toString(Shape::DEFAULT_IMG_WIDTH));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_HEIGHT,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_POSITIVE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Height of rendered image in meters", 
            toString(Shape::DEFAULT_IMG_HEIGHT));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_IMGFILE,
            ATTRPROPERTY_STRING | ATTRPROPERTY_FILENAME | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "A bitmap to use for rendering this poi", 
            toString(Shape::DEFAULT_IMG_FILE));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_RELATIVEPATH,
            ATTRPROPERTY_BOOL | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Enable or disable use image file as a relative path", 
            toString(Shape::DEFAULT_RELATIVEPATH));
        myAllowedAttributes[currentTag].addAttribute(SUMO_ATTR_ANGLE,
            ATTRPROPERTY_FLOAT | ATTRPROPERTY_ANGLE | ATTRPROPERTY_DEFAULTVALUE | ATTRPROPERTY_OPTIONAL,
            "Angle of rendered image in degree", 
            toString(Shape::DEFAULT_ANGLE));
    }
}

/****************************************************************************/

