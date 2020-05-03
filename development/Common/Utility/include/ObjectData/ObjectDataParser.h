///////////////////////////////////////////////////////////////////////
// ObjectDataParser.h
//
//  Copyright 10/26/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//  Internal file to read object data and create all the components from it
//
//
//  #include "ObjectData/ObjectDataParser.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef OBJECT_DATA_PARSER_H
#define OBJECT_DATA_PARSER_H
#pragma once
#if 0
// forward decleration

#include <Base.h>
#include <MuckLibMath.h>
//#include <Math/Vector.h>
#include <wx/string.h>
#include <vector>
#include <boost/smart_ptr.hpp>

class wxXmlNode;
class wxXmlDocument;
class wxXmlProperty;

namespace eg
{
	//provide helper typedefs
	namespace xml
	{
		typedef std::vector<wxXmlNode *> vNodeList_t;
		typedef vNodeList_t::iterator it_N;
		typedef vNodeList_t::const_iterator cit_N;

        typedef std::vector<wxXmlProperty *> vPropertiesList_t;
        typedef vPropertiesList_t::iterator it_P;
        typedef vPropertiesList_t::const_iterator cit_P;
	} //namespace xml

	/*!
    This will return xml node pointing to requested section.

    \param pCurrSection xml node to start search
    \param sectionName section name to find
    \return pointer to node with section name
    */
    wxXmlNode *FindSection(wxXmlNode *pCurrSection, const wxString& sectionName);

	/*!
	This will extract all the nodes which have the section name

	\param pCurrSection xml node to start search
	\param sectionName all section names to find
	\param sectionList [OUT] list of all wxXmlNode with section name
	\param bIncludeChildren if TRUE, then besides checking next, check all the children
	\return TRUE there were some wxXmlNode, otherwise FALSE and sectionList is not modified

	\note: This will step once into child, since you can pass parent to all of the sections.
	       Do we need to have extra bool param to control that?
	*/
	bool FindAllSections(wxXmlNode *pCurrSection, const wxString& sectionName, xml::vNodeList_t& sectionList, bool bIncludeChildren);

    /*!
    This will find all section in the entire xml node, regardless off name collision, since all
    section names are return in flat space

	\param pCurrSection xml node to start search
    \param sectionList [OUT] List of sections
    \return TRUE, there are some sections, otherwise FALSE and sectionList is left unchanged.
    */
    bool FindAllSections(wxXmlNode *pCurrSection, xml::vNodeList_t& sectionList);

    /*!
    This will return node for component of type
    \param pCurrSection xml node to start search
    \param objectName which object name to use to find the component. If "", then first come
    \param type type of the component to find
    \return pointer to node which contains component, or NULL if there wasn't one.
    */
    wxXmlNode *FindComponent(wxXmlNode *pCurrSection, const wxString& objectName, const wxString& type);

    /*!
    This is used to load object template from file.
    This will also free all of the template in ctor,
    so use it only fir the scope duration
    */
    class ObjectTemplateNode
    {
        //@{
        //! no copy semantics
        ObjectTemplateNode(const ObjectTemplateNode&);
        ObjectTemplateNode& operator=(const ObjectTemplateNode&);
        //@}

    public:
        ObjectTemplateNode(const wxString& templateName);

        //! Return node to object template
        wxXmlNode *GetNode() const;

    private:
        boost::scoped_ptr<wxXmlDocument> mpXmlDoc;
        wxXmlNode *mpNode;
    };

    /*!
    This is used to load object includes from file
    which are used to provide pre-built object data
    and it then it can also combine Include with existing
    node.
    This will also free all of the template in ctor,
    so use it only fir the scope duration
    */
    class ObjectIncludeNode
    {
    public:
        ObjectIncludeNode() : mpNode(0)
        {
        }

        ObjectIncludeNode(const wxString& includeName, wxXmlNode *pUserNode);
        //! Try to extract include properties from pUserNode if one exist
        ObjectIncludeNode(wxXmlNode *pUserNode, bool bCombine = true);

        //! Return node to object template
        wxXmlNode *GetNode() const;

    private:
        // helper load method to load the include object file
        bool Load(const wxString& includeName);
		// this will combine include file with UserNode if one provided
        void Combine(wxXmlNode *pUserNode);
        //! This will make sure that any components in pUserNode
        //! which are not in mpNode will be replicated
        void PreReplicate(wxXmlNode *pUserNode, wxXmlNode *pThisNode);

        boost::shared_ptr<wxXmlDocument> mpXmlDoc;
        wxXmlNode *mpNode;
    };

	Vector2 GetPropertyV2(wxXmlNode *pNode, const wxString& propName, const Vector2& defualtValue = Vector2(0, 0));
	Vector3 GetPropertyV3(wxXmlNode *pNode, const wxString& propName, const Vector3& defualtValue = Vector3(0, 0, 0));
	Vector4 GetPropertyV4(wxXmlNode *pNode, const wxString& propName, const Vector4& defualtValue = Vector4(0, 0, 0, 0));
	wxString GetProperty(wxXmlNode *pNode, const wxString& propName, const wxString& defualtValue = wxEmptyString);
	int GetPropertyInt(wxXmlNode *pNode, const wxString& propName, int defualtValue = -1);
	float GetPropertyFloat(wxXmlNode *pNode, const wxString& propName, float defualtValue = 0.0f);
    bool GetPropertyBool(wxXmlNode *pNode, const wxString& propName, bool defualtValue = false);

    //! Set new name on this object template. If there are more then 1 object section
    //! they will be disreagarded.
    void SetObjectName(wxXmlNode *pNode, const wxString& newName);
    //! Return object name extracted from xml node
    wxString GetObjectName(wxXmlNode *pNode);
    //! Return Object Name by loading xmlFileName and calling GetObjectName(wxXmlNode *pNode)
    wxString GetObjectName(const wxString& xmlFileName);

    /*!
    This allows us to set property to new value

    \param pNode xml node to start looking for sectionName.
    \param sectionName name of the section. This can be path "SomeName/AnotherName"
    \param propName property name to change
    \param newValue new value of propety
    \return wxString of the prevoius property value

    \note If there is no propert, then it will do nothin and return ""
    \todo EG: Should we add option to create property if one does not exists?
    */
    wxString SetProperty(wxXmlNode *pNode, const wxString& sectionName, const wxString& propName, const wxString& newValue);


} // namespace eg
#endif // 0
       //
#endif // OBJECT_DATA_PARSER_H

