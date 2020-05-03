#include "ObjectData/ObjectDataParser.h"
#if 0
#include <Config/ConfigHelper.h>
#include <wx/confbase.h>
#include <wx/filename.h>
#include <wx/xml/xml.h>
#include <wx/tokenzr.h>
#include <boost/bind.hpp>

namespace
{
    bool IsType(wxXmlNode *pNode, const wxString& name)
    {
        if (pNode)
        {
            wxString type = pNode->GetPropVal(_T("type"), _T(""));
            if (type.IsSameAs(name, false))
            {
                return true;
            }
        }

        return false;
    }
} // namespace

namespace eg {


wxXmlNode *FindSection(wxXmlNode *pCurrSection, const wxString& sectionName)
{
    if (pCurrSection)
    {
        if (pCurrSection->GetName().IsSameAs(sectionName, false))
        {
            return pCurrSection;
        }

        if (pCurrSection->GetChildren())
        {
            if (wxXmlNode *pNode = FindSection(pCurrSection->GetChildren(), sectionName))
            {
                return pNode;
            }
        }

        // this means that we are object tag or no childer and no object tag
        if (pCurrSection->GetNext())
        {
            if (wxXmlNode *pNode = FindSection(pCurrSection->GetNext(), sectionName))
            {
                return pNode;
            }
        }
    }

    return 0;
}


bool FindAllSections(wxXmlNode *pCurrSection, const wxString& sectionName, xml::vNodeList_t& sectionList, bool bIncludeChildren)
{
    wxASSERT_MSG(bIncludeChildren == false, "bIncludeChildren == true is not implemented yet!");
    size_t numLastSections = sectionList.size();

    if (pCurrSection && !pCurrSection->GetName().IsSameAs(sectionName, false))
    {
        // if we not at the section name, than this is it's parent, so let's step once into it
        pCurrSection = pCurrSection->GetChildren();
    }

    while (pCurrSection)
    {
        // now we at the level where all we need to do is to iterate wxXmlNode::GetNext()
        if (pCurrSection->GetName().IsSameAs(sectionName, false))
        {
            sectionList.push_back(pCurrSection);
        }

        pCurrSection = pCurrSection->GetNext();
    }

    return sectionList.size() > numLastSections;
}



// add bool to specify if we want to include siblings of original node (pCurrSection)
bool FindAllSections(wxXmlNode *pCurrSection, xml::vNodeList_t& sectionList)
{
    size_t numLastSections = sectionList.size();

    if (pCurrSection->GetType() == wxXML_ELEMENT_NODE)
    {
        sectionList.push_back(pCurrSection);
    }

    if (pCurrSection->GetChildren())
    {
        FindAllSections(pCurrSection->GetChildren(), sectionList);
    }

    if (pCurrSection->GetNext())
    {
        FindAllSections(pCurrSection->GetNext(), sectionList);
    }

    return sectionList.size() > numLastSections;
}


bool FindAllProperties(wxXmlNode *pCurrSection, xml::vPropertiesList_t& propertiesList)
{
    size_t numLastProperties = propertiesList.size();

    wxXmlProperty *pCurrProperty = pCurrSection->GetProperties();
    while (pCurrProperty)
    {
        propertiesList.push_back(pCurrProperty);
        pCurrProperty = pCurrProperty->GetNext();
    }

    return propertiesList.size() > numLastProperties;
}


ObjectTemplateNode::ObjectTemplateNode(const wxString& templateName) :
    mpXmlDoc(new wxXmlDocument),
    mpNode(0)
{
    wxString objectFileName = eg::ConfigReadPath(wxConfigBase::Get(false), _T("Data Folders/Objects")) + _T("Templates/") + templateName;
    if (mpXmlDoc->Load(objectFileName))
    {
        mpNode = FindSection(mpXmlDoc->GetRoot(), _T("object"));
    }
}


wxXmlNode *ObjectTemplateNode::GetNode() const
{
    return mpNode;
}


ObjectIncludeNode::ObjectIncludeNode(const wxString& includeName, wxXmlNode *pUserNode) :
    mpNode(0)
{
    if (Load(includeName) && pUserNode)
    {
        Combine(pUserNode);
    }
}


ObjectIncludeNode::ObjectIncludeNode(wxXmlNode *pUserNode, bool bCombine) :
    mpNode(0)
{
    wxString includeName = GetProperty(pUserNode, _T("include"));
    if (Load(includeName) && bCombine)
    {
        Combine(pUserNode);
    }
}


wxXmlNode *ObjectIncludeNode::GetNode() const
{
    return mpNode;
}


bool ObjectIncludeNode::Load(const wxString& includeName)
{
    wxString objectFileName = eg::ConfigReadPath(wxConfigBase::Get(false), _T("Data Folders/Objects")) + includeName;
    if (wxFileName::FileExists(objectFileName))
    {
        mpXmlDoc.reset(new wxXmlDocument);
        // let's make sure that we have this file
        if (mpXmlDoc->Load(objectFileName))
        {
            mpNode = FindSection(mpXmlDoc->GetRoot(), _T("object"));
        }
    }

    return mpNode != 0;
}


void ObjectIncludeNode::PreReplicate(wxXmlNode *pUserNode, wxXmlNode *pThisNode)
{
    if (pUserNode && pThisNode)
    {
        xml::vNodeList_t userComponentsList;
        xml::vNodeList_t thisComponentsList;
        FindAllSections(pUserNode, _T("component"), userComponentsList, false);
        FindAllSections(pThisNode, _T("component"), thisComponentsList, false);
        // for every components in userComponentsList, we must make sure that it
        // exists in thisComponentsList, and if not replicate it
        for (xml::cit_N it_user = userComponentsList.begin(); it_user != userComponentsList.end(); ++it_user)
        {
            wxString type = (*it_user)->GetPropVal(_T("type"), _T(""));

            xml::cit_N it_name = std::find_if(thisComponentsList.begin(), thisComponentsList.end(),
                                              boost::bind(IsType, _1, type));
            if (it_name == thisComponentsList.end())
            {
                // we do not have user section in this, so let's add it
                wxXmlNode *pNewNode = new wxXmlNode(*(*it_user));
                pThisNode->AddChild(pNewNode);
            }
        }
    }
}


void ObjectIncludeNode::Combine(wxXmlNode *pUserNode)
{
    // what we are doing here is extracting all the properties from pUserNode
    // and applying those values to our internal node
    // which allows us to combine user node with our internal stuff.
    // let's extract object name from user node and use that for our internal mpNode
    wxXmlNode *pObjectNode = FindSection(pUserNode, _T("object"));
    wxString objectName = GetProperty(pObjectNode, _T("name"));
    if (!objectName.empty())
    {
        SetProperty(mpNode, _T("object"), _T("name"), objectName);
    }

    wxLogDebug("Processing include for Object: %s", objectName.c_str());

    PreReplicate(pUserNode, FindSection(mpNode, _T("object")));
    // next grab all the components
    xml::vNodeList_t componentsList;
    if (FindAllSections(pUserNode, _T("component"), componentsList, false))
    {
        for (xml::it_N it_node = componentsList.begin(); it_node != componentsList.end(); ++it_node)
        {
            // the for each component we need to extract all the sections
            xml::vNodeList_t sectionList;
            if (FindAllSections(*it_node, sectionList))
            {
                for (xml::it_N it_section = sectionList.begin(); it_section != sectionList.end(); ++it_section)
                {
                    // user can remove entire component
                    if ((*it_section)->GetName() == _T("component"))
                    {
                        if (GetPropertyBool(*it_section, _T("remove"), false))
                        {
                            // looks like we need to remove this entire component
                            wxString componentType = GetProperty(*it_section, _T("type"));
                            if (wxXmlNode *pComponentToRemove = FindComponent(mpNode, _T(""), componentType))
                            {
                                // we have component to remove, so grab the parent and then
                                // remove this one
                                if (wxXmlNode *pParent = pComponentToRemove->GetParent())
                                {
                                    wxLogDebug("Removing Component: %s", componentType.c_str());
                                    pParent->RemoveChild(pComponentToRemove);
                                    delete pComponentToRemove;
                                    continue;
                                }
                            }
                        }
                    }

                    // for each section we need to extract all the properties for it
                    xml::vPropertiesList_t propertiesList;
                    if (FindAllProperties(*it_section, propertiesList))
                    {
                        for (xml::it_P it_prop = propertiesList.begin(); it_prop != propertiesList.end(); ++it_prop)
                        {
                            // if node and section are the same we can skip it
                            if ((*it_node)->GetName() != (*it_section)->GetName())
                            {
                                wxLogDebug("Including Node: %s, Section: %s, : PropName: %s, PropValue: %s", (*it_node)->GetName(), (*it_section)->GetName(), (*it_prop)->GetName(), (*it_prop)->GetValue());
                                SetProperty(mpNode, (*it_section)->GetName(), (*it_prop)->GetName(), (*it_prop)->GetValue());
                            }
                        }
                    }
                }
            }
        }
    }
}


Vector2 GetPropertyV2(wxXmlNode *pNode, const wxString& propName, const Vector2& defualtValue)
{
    return Vector2(GetPropertyV4(pNode, propName, Vector4(defualtValue)));
}


Vector3 GetPropertyV3(wxXmlNode *pNode, const wxString& propName, const Vector3& defualtValue)
{
    return Vector3(GetPropertyV4(pNode, propName, Vector4(defualtValue, 0.0f)));
}


Vector4 GetPropertyV4(wxXmlNode *pNode, const wxString& propName, const Vector4& defualtValue)
{
    Vector4 value = defualtValue;
    wxString buffer;
    if (pNode->GetPropVal(propName, &buffer))
    {
        value.set(buffer.c_str());
    }

    return value;
}


int GetPropertyInt(wxXmlNode *pNode, const wxString& propName, int defualtValue)
{
    long value = defualtValue;
    wxString buffer;
    if (pNode->GetPropVal(propName, &buffer))
    {
        buffer.ToLong(&value);
    }

    return value;
}


float GetPropertyFloat(wxXmlNode *pNode, const wxString& propName, float defualtValue)
{
    double value = defualtValue;
    wxString buffer;
    if (pNode->GetPropVal(propName, &buffer))
    {
        buffer.ToDouble(&value);
    }

    return value;
}


bool GetPropertyBool(wxXmlNode *pNode, const wxString& propName, bool defualtValue)
{
    bool value = defualtValue;
    wxString buffer;
    if (pNode->GetPropVal(propName, &buffer))
    {
        if (buffer.IsSameAs(_T("TRUE"), false))
        {
            value = true;
        }
        else if (buffer.IsSameAs(_T("FALSE"), false))
        {
            value = false;
        }
    }

    return value;
}


wxString GetProperty(wxXmlNode *pNode, const wxString& propName, const wxString& defualtValue)
{
    wxString value = defualtValue;
    pNode->GetPropVal(propName, &value);
    return value;
}


wxString SetProperty(wxXmlNode *pNode, const wxString& sectionName, const wxString& propName, const wxString& newValue)
{
    wxXmlNode *pRequestedNode = pNode;
    wxStringTokenizer tokens(sectionName, _T("/"));

    wxString currName;
    while ((currName = tokens.GetNextToken()) != wxEmptyString)
    {
        if (wxXmlNode *pNewNode = FindComponent(pRequestedNode, _T(""), currName))
        {
            pRequestedNode = pNewNode;
        }
        else
        {
            pRequestedNode = FindSection(pRequestedNode, currName);
        }

    }

    if (pRequestedNode)
    {
        wxXmlProperty *pXmlProperty = pRequestedNode->GetProperties();
        while (pXmlProperty)
        {
            if (pXmlProperty->GetName().IsSameAs(propName, false))
            {
                wxString oldValue = pXmlProperty->GetValue();
                pXmlProperty->SetValue(newValue);
                return oldValue;
            }

            pXmlProperty = pXmlProperty->GetNext();
        }

        // it looks like there is no property of this kind, let's add it.
        pRequestedNode->AddProperty(propName, newValue);
    }

    return wxEmptyString;
}


wxXmlNode *FindComponent(wxXmlNode *pCurrSection, const wxString& objectName, const wxString& type)
{
    wxXmlNode *pObjectNode = 0;

    if (objectName.empty())
    {
        // first object section we come across, since
        // user did not specify it
        pObjectNode = FindSection(pCurrSection, _T("object"));
    }
    else
    {
        // find specific object section
        xml::vNodeList_t objectList;
        if (FindAllSections(pCurrSection, _T("object"), objectList, false))
        {
            for (xml::cit_N it = objectList.begin(); it != objectList.end(); ++it)
            {
                wxString componentType = GetProperty(*it, _T("name"));
                if (componentType.IsSameAs(objectName, false))
                {
                    pObjectNode = *it;
                    break;
                }
            }
        }
    }

    // if we have that object section, let's find requested component
    if (pObjectNode)
    {
        xml::vNodeList_t componentsList;
        if (FindAllSections(pObjectNode, _T("component"), componentsList, false))
        {
            // now we need to create all the components for this object,
            for (xml::it_N itc = componentsList.begin(); itc != componentsList.end(); ++itc)
            {
                wxString componentType = GetProperty(*itc, _T("type"));
                if (componentType.IsSameAs(type, false))
                {
                    return *itc;
                }
            }
        }
    }

    return 0;
}


void SetObjectName(wxXmlNode *pNode, const wxString& newName)
{
    SetProperty(pNode, _T("object"), _T("name"), newName);
}


wxString GetObjectName(wxXmlNode *pNode)
{
    if (wxXmlNode *pObjectNode = FindSection(pNode, _T("object")))
    {
        wxString objectName = GetProperty(pObjectNode, _T("name"));
        return objectName;
    }

    return wxEmptyString;
}


wxString GetObjectName(const wxString& xmlFileName)
{
    ObjectIncludeNode objectNode(xmlFileName, 0);
    return GetObjectName(objectNode.GetNode());
}

} // namespace eg
#endif // 0

