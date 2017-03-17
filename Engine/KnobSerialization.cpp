/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "KnobSerialization.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp> // iequals
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"



NATRON_NAMESPACE_ENTER;

ValueSerialization::ValueSerialization()
: _serialization(0)
, _knob()
, _dimension(0)
, _master()
, _expression()
, _exprHasRetVar(false)
{

}

ValueSerialization::ValueSerialization(KnobSerializationBase* serialization,
                                       const KnobPtr & knob,
                                       int dimension)
    : _serialization(serialization)
    , _knob(knob)
    , _dimension(dimension)
    , _master()
    , _expression()
    , _exprHasRetVar(false)
{
}

void
ValueSerialization::initForLoad(KnobSerializationBase* serialization,
                 const KnobPtr & knob,
                 int dimension)
{
    _serialization = serialization;
    _knob = knob;
    _dimension = dimension;
}

ValueSerialization::ValueSerialization(const KnobPtr & knob,
                                       int dimension,
                                       bool exprHasRetVar,
                                       const std::string& expr)
    : _serialization(0)
    , _knob()
    , _dimension(0)
    , _master()
    , _expression()
    , _exprHasRetVar(false)
{
    initForSave(knob, dimension, exprHasRetVar, expr);
}

void
ValueSerialization::initForSave(const KnobPtr & knob,
                 int dimension,
                 bool exprHasRetVar,
                 const std::string& expr)
{
    _knob = knob;
    _dimension = dimension;
    _expression = expr;
    _exprHasRetVar = exprHasRetVar;

    std::pair< int, KnobPtr > m = knob->getMaster(dimension);

    if ( m.second && !knob->isMastersPersistenceIgnored() ) {
        _master.masterDimension = m.first;
        NamedKnobHolder* holder = dynamic_cast<NamedKnobHolder*>( m.second->getHolder() );
        assert(holder);

        TrackMarker* isMarker = dynamic_cast<TrackMarker*>(holder);
        if (isMarker) {
            _master.masterTrackName = isMarker->getScriptName_mt_safe();
            _master.masterNodeName = isMarker->getContext()->getNode()->getScriptName_mt_safe();
        } else {
            // coverity[dead_error_line]
            _master.masterNodeName = holder ? holder->getScriptName_mt_safe() : "";
        }
        _master.masterKnobName = m.second->getName();
    } else {
        _master.masterDimension = -1;
    }

}

void
ValueSerialization::setChoiceExtraLabel(const std::string& label)
{
    assert(_serialization);
    _serialization->setChoiceExtraString(label);
}

KnobPtr
KnobSerialization::createKnob(const std::string & typeName,
                              int dimension)
{
    KnobPtr ret;

    if ( typeName == KnobInt::typeNameStatic() ) {
        ret.reset( new KnobInt(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobBool::typeNameStatic() ) {
        ret.reset( new KnobBool(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobDouble::typeNameStatic() ) {
        ret.reset( new KnobDouble(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobChoice::typeNameStatic() ) {
        ret.reset( new KnobChoice(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobString::typeNameStatic() ) {
        ret.reset( new KnobString(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobParametric::typeNameStatic() ) {
        ret.reset( new KnobParametric(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobColor::typeNameStatic() ) {
        ret.reset( new KnobColor(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobPath::typeNameStatic() ) {
        ret.reset( new KnobPath(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobLayers::typeNameStatic() ) {
        ret.reset( new KnobLayers(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobFile::typeNameStatic() ) {
        ret.reset( new KnobFile(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobOutputFile::typeNameStatic() ) {
        ret.reset( new KnobOutputFile(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobButton::typeNameStatic() ) {
        ret.reset( new KnobButton(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobSeparator::typeNameStatic() ) {
        ret.reset( new KnobSeparator(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobGroup::typeNameStatic() ) {
        ret.reset( new KnobGroup(NULL, std::string(), dimension, false) );
    } else if ( typeName == KnobPage::typeNameStatic() ) {
        ret.reset( new KnobPage(NULL, std::string(), dimension, false) );
    }

    if (ret) {
        ret->populate();
    }

    return ret;
}

static KnobPtr
findMaster(const KnobPtr & knob,
           const NodesList & allNodes,
           const std::string& masterKnobName,
           const std::string& masterNodeName,
           const std::string& masterTrackName,
           const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    ///we need to cycle through all the nodes of the project to find the real master
    NodePtr masterNode;
    std::string masterNodeNameToFind = masterNodeName;

    /*
       When copy pasting, the new node copied has a script-name different from what is inside the serialization because 2
       nodes cannot co-exist with the same script-name. We keep in the map the script-names mapping
     */
    std::map<std::string, std::string>::const_iterator foundMapping = oldNewScriptNamesMapping.find(masterNodeName);

    if ( foundMapping != oldNewScriptNamesMapping.end() ) {
        masterNodeNameToFind = foundMapping->second;
    }

    for (NodesList::const_iterator it2 = allNodes.begin(); it2 != allNodes.end(); ++it2) {
        if ( (*it2)->getScriptName() == masterNodeNameToFind ) {
            masterNode = *it2;
            break;
        }
    }
    if (!masterNode) {
        qDebug() << "Link slave/master for " << knob->getName().c_str() <<   " failed to restore the following linkage: " << masterNodeNameToFind.c_str();

        return KnobPtr();
    }

    if ( !masterTrackName.empty() ) {
        boost::shared_ptr<TrackerContext> context = masterNode->getTrackerContext();
        if (context) {
            TrackMarkerPtr marker = context->getMarkerByName(masterTrackName);
            if (marker) {
                return marker->getKnobByName(masterKnobName);
            }
        }
    } else {
        ///now that we have the master node, find the corresponding knob
        const std::vector< KnobPtr > & otherKnobs = masterNode->getKnobs();
        for (std::size_t j = 0; j < otherKnobs.size(); ++j) {
            if ( (otherKnobs[j]->getName() == masterKnobName) && otherKnobs[j]->getIsPersistent() ) {
                return otherKnobs[j];
                break;
            }
        }
    }

    qDebug() << "Link slave/master for " << knob->getName().c_str() <<   " failed to restore the following linkage: " << masterNodeNameToFind.c_str();

    return KnobPtr();
}

void
KnobSerialization::restoreKnobLinks(const KnobPtr & knob,
                                    const NodesList & allNodes,
                                    const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    int i = 0;

    if (_masterIsAlias) {
        /*
         * _masters can be empty for example if we expand a group: the slaved knobs are no longer slaves
         */
        if ( !_masters.empty() ) {
            const std::string& aliasKnobName = _masters.front().masterKnobName;
            const std::string& aliasNodeName = _masters.front().masterNodeName;
            const std::string& masterTrackName  = _masters.front().masterTrackName;
            KnobPtr alias = findMaster(knob, allNodes, aliasKnobName, aliasNodeName, masterTrackName, oldNewScriptNamesMapping);
            if (alias) {
                knob->setKnobAsAliasOfThis(alias, true);
            }
        }
    } else {
        for (std::list<MasterSerialization>::iterator it = _masters.begin(); it != _masters.end(); ++it) {
            if (it->masterDimension != -1) {
                KnobPtr master = findMaster(knob, allNodes, it->masterKnobName, it->masterNodeName, it->masterTrackName, oldNewScriptNamesMapping);
                if (master) {
                    knob->slaveTo(i, master, it->masterDimension);
                }
            }
            ++i;
        }
    }
}

void
KnobSerialization::restoreExpressions(const KnobPtr & knob,
                                      const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    int dims = std::min( knob->getDimension(), _knob->getDimension() );

    try {
        for (int i = 0; i < dims; ++i) {
            if ( !_expressions[i].first.empty() ) {
                QString expr( QString::fromUtf8( _expressions[i].first.c_str() ) );

                //Replace all occurrences of script-names that we know have changed
                for (std::map<std::string, std::string>::const_iterator it = oldNewScriptNamesMapping.begin();
                     it != oldNewScriptNamesMapping.end(); ++it) {
                    expr.replace( QString::fromUtf8( it->first.c_str() ), QString::fromUtf8( it->second.c_str() ) );
                }
                knob->restoreExpression(i, expr.toStdString(), _expressions[i].second);
            }
        }
    } catch (const std::exception& e) {
        QString err = QString::fromUtf8("Failed to restore expression: %1").arg( QString::fromUtf8( e.what() ) );
        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( knob->getName().c_str() ), QDateTime::currentDateTime(), err);
    }
}

void
KnobSerialization::setChoiceExtraString(const std::string& label)
{
    assert(_extraData);
    ChoiceExtraData* cData = dynamic_cast<ChoiceExtraData*>(_extraData);
    assert(cData);
    if (cData) {
        cData->_choiceString = label;
    }
}

void
KnobSerialization::checkForPreNatron226String(std::string* choiceString)
{
    if (boost::iequals(*choiceString,std::string("Color.RGBA")) || boost::iequals(*choiceString,std::string("Color.RGB")) || boost::iequals(*choiceString,std::string("Color.Alpha"))) {
        *choiceString = kNatronColorPlaneID;
    } else if (boost::iequals(*choiceString,std::string("Backward.Motion"))) {
        *choiceString = kNatronBackwardMotionVectorsPlaneID "." kNatronMotionComponentsLabel;
    } else if (boost::iequals(*choiceString,std::string("Forward.Motion"))) {
        *choiceString = kNatronForwardMotionVectorsPlaneID "." kNatronMotionComponentsLabel;
    } else if (boost::iequals(*choiceString, std::string("DisparityLeft.Disparity"))) {
        *choiceString = kNatronDisparityLeftPlaneID "." kNatronDisparityComponentsLabel;
    } else if (boost::iequals(*choiceString, std::string("DisparityRight.Disparity"))) {
        *choiceString = kNatronDisparityRightPlaneID "." kNatronDisparityComponentsLabel;
    }

    // Map also channels
    if (boost::iequals(*choiceString, std::string("RGBA.R")) || boost::iequals(*choiceString, std::string("UV.r"))) {
        *choiceString = kNatronColorPlaneID ".R";
    } else if (boost::iequals(*choiceString, std::string("RGBA.G")) || boost::iequals(*choiceString, std::string("UV.g"))) {
        *choiceString = kNatronColorPlaneID ".G";
    } else if (boost::iequals(*choiceString, std::string("RGBA.B")) || boost::iequals(*choiceString, std::string("UV.b"))) {
        *choiceString = kNatronColorPlaneID ".B";
    } else if (boost::iequals(*choiceString, std::string("RGBA.A")) || boost::iequals(*choiceString, std::string("UV.a"))) {
        *choiceString = kNatronColorPlaneID ".A";
    } else if (boost::iequals(*choiceString, std::string("A.r"))) {
        *choiceString = "A." kNatronColorPlaneID ".r";
    } else if (boost::iequals(*choiceString, std::string("A.g"))) {
        *choiceString = "A." kNatronColorPlaneID ".g";
    } else if (boost::iequals(*choiceString, std::string("A.b"))) {
        *choiceString = "A." kNatronColorPlaneID ".b";
    } else if (boost::iequals(*choiceString, std::string("A.a"))) {
        *choiceString = "A." kNatronColorPlaneID ".a";
    } else if (boost::iequals(*choiceString, std::string("B.r"))) {
        *choiceString = "B." kNatronColorPlaneID ".r";
    } else if (boost::iequals(*choiceString, std::string("B.g"))) {
        *choiceString = "B." kNatronColorPlaneID ".g";
    } else if (boost::iequals(*choiceString, std::string("B.b"))) {
        *choiceString = "B." kNatronColorPlaneID ".b";
    } else if (boost::iequals(*choiceString, std::string("B.a"))) {
        *choiceString = "B." kNatronColorPlaneID ".a";
    }
}

NATRON_NAMESPACE_EXIT;
