///////////////////////////////////////////////////////////////////////
// PropertyDataImpl.h
//
//  Copyright 11/11/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Inline implementation if Property data system
//
//
//  #include "PropertyDataImpl.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef PROPERTY_PROPERTY_DATA_IMPL_H
#define PROPERTY_PROPERTY_DATA_IMPL_H

#ifndef PROPERTY_PROPERTY_DATA_INCLUDE_IMPLEMENTATION
    #error "Do not include this file explicitly."
#endif // PROPERTY_PROPERTY_DATA_INCLUDE_IMPLEMENTATION

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace eg
{
    // ------------------------------------------------------------------------------------------------
    // Property Begin (Property : public IProperty)
    // ------------------------------------------------------------------------------------------------
    template <typename T>
	inline Property<T>::Property(PropertySet *pPropertySet, const std::string& name, T *value, bool readOnly, PropertyChangedCallback_t propertyChangedCallback, const std::string& typeOptions)
    : mName(name)
    , mValue(value)
    , mReadOnly(readOnly)
    , mpPropertySet(pPropertySet)
    , mPropertyChangedCallback(propertyChangedCallback)
    , mPropId(Hash(name.c_str()))
	, mPropTypeOptions(typeOptions)
    {
    }

    template <typename T>
    inline Property<T>::~Property() {}

    template <typename T>
    inline uint32_t Property<T>::GetPropId() const
    {
        return mPropId;
    }

    template <typename T>
    inline const std::string& Property<T>::GetName() const
    {
        return mName;
    }

    template <typename T>
    inline std::string Property<T>::GetValue() const
    {
        return ConvertProperty<T>(*mValue);
    }

    template <typename T>
    inline void Property<T>::SetValue(const std::string& newValue)
    {
        if (!mReadOnly)
        {
            T cnvValue = ConvertProperty<T>(newValue);
			if (*mValue == cnvValue)
			{
				return;
			}

			(*mValue) = cnvValue;
            if (mPropertyChangedCallback)
            {
                mPropertyChangedCallback(this);
            }
        }
    }

    template <typename T>
    inline bool Property<T>::IsReadOnly() const
    {
        return mReadOnly;
    }

    template <typename T>
    inline const std::type_info& Property<T>::GetPropType() const
    {
        return typeid(T);
    }

	template <typename T>
	inline const std::string& Property<T>::GetPropTypeOptions() const
	{
		return mPropTypeOptions;
	}

    template <typename T>
    const std::type_info& Property<T>::GetPropValue() const
    {
        return typeid(T);
    }

    template <typename T>
    inline component::InstanceId Property<T>::GetInstanceId() const
    {
        if (mpPropertySet)
        {
            return mpPropertySet->GetInstanceId();
        }

        return component::InstanceId();
    }

    template <typename T>
    inline PropertyChangedCallback_t Property<T>::SetChangeCallback(PropertyChangedCallback_t propertyChangedCallback)
    {
        PropertyChangedCallback_t retValue = mPropertyChangedCallback;
        mPropertyChangedCallback = propertyChangedCallback;
        return retValue;
    }

    template <typename T>
    inline const T& Property<T>::operator()() const
    {
        if (mValue)
        {
            return *mValue;
        }

        wxLogWarning("Trying to get Value but it's not initialized for '%s' property", GetName().c_str());
        static T emptyValue;
        return emptyValue;
    }

    template <typename T>
    inline T& Property<T>::operator()()
    {
        if (mValue && !mReadOnly)
        {
            return *mValue;
        }

        wxASSERT_MSG(0, "Trying to get non const Value on a Read Only property");
        wxLogError("Trying to get non const Value on a Read Only '%s' property", GetName().c_str());
        static T emptyValue;
        return emptyValue;
    }

    template <typename T>
    inline Property<T>& Property<T>::operator =(const T& value)
    {
        if (!mReadOnly)
        {
			// if new value and existing are the same, early out
			if (*mValue == value)
			{
				return *this;
			}

            *mValue = value;
            if (mPropertyChangedCallback)
            {
                mPropertyChangedCallback(this);
            }
        }
        else
        {
            wxASSERT_MSG(0, "Trying to assign new Value to Read Only property");
            wxLogError("Trying to assign new Value to Read Only '%s' property", GetName().c_str());
        }

        return *this;
    }

    template <typename T>
    inline Property<T>::operator T() const
    {
        return *mValue;
    }

    template <typename T>
    inline void Property<T>::SetNativeValue(const T& value)
    {
        (*this) = value;
    }


    // ------------------------------------------------------------------------------------------------
    // Property End (Property : public IProperty)
    // ------------------------------------------------------------------------------------------------


    // ------------------------------------------------------------------------------------------------
    // PropertyGeneric Begin (PropertyGeneric : public Property)
    // ------------------------------------------------------------------------------------------------
    template <typename T, typename V>
	inline PropertyGeneric<T, V>::PropertyGeneric(PropertySet *pPropertySet, const std::string& name, V *value, bool readOnly, PropertyChangedCallback_t propertyChangedCallback, const std::string& typeOptions)
    : Property<V>(pPropertySet, name, value, readOnly, propertyChangedCallback, typeOptions)
    {

    }

    template <typename T, typename V>
    inline PropertyGeneric<T, V>::~PropertyGeneric() {}

    template <typename T, typename V>
    inline const std::type_info& PropertyGeneric<T, V>::GetPropType() const
    {
        return typeid(T);
    }

    template <typename T, typename V>
    const std::type_info& PropertyGeneric<T, V>::GetPropValue() const
    {
        return typeid(V);
    }

    template <typename T, typename V>
    inline PropertyGeneric<T, V>& PropertyGeneric<T, V>::operator =(const V& value)
    {
        dynamic_cast<Property<V>& >(*this) = value;
        return *this;
    }

    template <typename T, typename V>
    inline const PropertyGeneric<T, V>& PropertyGeneric<T, V>::operator =(const V& value) const
    {
        dynamic_cast<const Property<V>& >(*this) = value;
        return *this;
    }

    template <typename T, typename V>
    inline PropertyGeneric<T, V>::operator V() const
    {
        return mValue;
    }

    // ------------------------------------------------------------------------------------------------
    // PropertyGeneric End (PropertyGeneric : public Property)
    // ------------------------------------------------------------------------------------------------


    // ------------------------------------------------------------------------------------------------
    // PropertyOwn Begin (PropertyOwn : public PropertyGeneric)
    // ------------------------------------------------------------------------------------------------
    template <typename T, typename V>
    inline PropertyOwn<T, V>& PropertyOwn<T, V>::operator =(const V& value)
    {
        dynamic_cast<Property<V>& >(*this) = value;
        return *this;
    }

    template <typename T, typename V>
    inline const PropertyOwn<T, V>& PropertyOwn<T, V>::operator =(const V& value) const
    {
        dynamic_cast<const Property<V>& >(*this) = value;
        return *this;
    }

    template <typename T, typename V>
    inline PropertyOwn<T, V>::operator V() const
    {
        return mValue;
    }
    // ------------------------------------------------------------------------------------------------
    // PropertyOwn End (PropertyOwn : public PropertyGeneric)
    // ------------------------------------------------------------------------------------------------


    // ------------------------------------------------------------------------------------------------
    // PropertySet Begin (PropertySet : public IPropertySet)
    // ------------------------------------------------------------------------------------------------
    inline PropertySet::PropertySet()
    : mPropertyChangedCallback(PropertyChangedCallback_t())
    {
        mProp_it = mPopertyData.end();
    }

    inline PropertySet::PropertySet(const component::InstanceId& instanceId, const std::string& groupName, PropertyChangedCallback_t propertyChangedCallback)
    : mGroupName(groupName)
    , mPropertyChangedCallback(propertyChangedCallback)
    , mInstanceId(instanceId)
    {
        mProp_it = mPopertyData.end();
    }

    inline PropertySet::~PropertySet()
    {
    }

    inline bool PropertySet::IsValid() const
    {
        return !mPopertyData.empty();
    }

    inline const std::string& PropertySet::GetGroupName() const
    {
        return mGroupName;
    }

    inline IProperty *PropertySet::GetFirstProperty() const
    {
        mProp_it = mPopertyData.begin();
        return GetNextProperty();
    }

    inline IProperty *PropertySet::GetNextProperty() const
    {
        if (mProp_it != mPopertyData.end())
        {
            IProperty *pCurrProp = (*mProp_it).second.get();
            ++mProp_it;
            return pCurrProp;
        }
        return 0;
    }

    inline IProperty *PropertySet::FindProperty(const std::string& propName) const
    {
        PopertyData_t::const_iterator it = mPopertyData.find(propName);
        if (it != mPopertyData.end())
        {
            return const_cast<IProperty *>((*it).second.get());
        }

        return 0;
    }

    /*
    inline void PropertySet::SetInstanceId(ComponentInstanceId_t instanceId)
    {
        mInstanceId = instanceId;
    }
    */

    inline component::InstanceId PropertySet::GetInstanceId() const
    {
        return mInstanceId;
    }

    inline void PropertySet::SetGroupName(const std::string& groupName)
    {
        mGroupName = groupName;
    }

    template <typename T, typename V>
	inline void PropertySet::AddProperty(const std::string name, V *value, bool readOnly, PropertyChangedCallback_t propertyChangedCallback, const std::string& typeOptions)
    {
        PopertyData_t::iterator it = mPopertyData.find(name);
        wxASSERT(it == mPopertyData.end());
        if (it == mPopertyData.end())
        {
            IProperty *pNewProp = new PropertyGeneric<T, V>(this, name, value, readOnly, propertyChangedCallback ? propertyChangedCallback : mPropertyChangedCallback, typeOptions);
            mPopertyData.insert(std::make_pair(name, pNewProp));
        }
    }

    template <typename T, typename V>
    inline void PropertySet::AddProperty(const std::string name, const V& value, PropertyChangedCallback_t propertyChangedCallback, const std::string& typeOptions)
    {
        PopertyData_t::iterator it = mPopertyData.find(name);
        wxASSERT(it == mPopertyData.end());
        if (it == mPopertyData.end())
        {
            IProperty *pNewProp = new PropertyOwn<T, V>(this, name, value, propertyChangedCallback ? propertyChangedCallback : mPropertyChangedCallback, typeOptions);
            mPopertyData.insert(std::make_pair(name, pNewProp));
        }
    }

    inline IProperty& PropertySet::operator[](const std::string& propName)
    {
        PopertyData_t::iterator it = mPopertyData.find(propName);
        wxASSERT(it != mPopertyData.end());
        IProperty *pProperty = (*it).second.get();
        return *pProperty;
    }

    inline const IProperty& PropertySet::operator[](const std::string& propName) const
    {
        PopertyData_t::const_iterator cit = mPopertyData.find(propName);
        wxASSERT(cit != mPopertyData.end());
        const IProperty *pProperty = (*cit).second.get();
        return *pProperty;
    }

	inline void PropertySet::AddNotifySlot(const IPropertySet::NotifySlot_t& slot, const std::string& userSlotName)
    {
        mNotifier.AddNotifySlot(slot, userSlotName, *this);
    }

    inline void PropertySet::RemoveNotifySlot(const std::string& userSlotName)
    {
        mNotifier.RemoveNotifySlot(userSlotName, *this);
    }

    inline void PropertySet::Notifier::AddNotifySlot(const IPropertySet::NotifySlot_t& slot, const std::string& userSlotName, PropertySet& propertySet)
    {
        if (mLocalPropertyCallbacks.empty())
        {
            // there is no global callbacks setup yet, so we need to extract local callback from each property,
            // store it in out map container and replace it with our own
            for (PopertyData_t::iterator it = propertySet.mPopertyData.begin(); it != propertySet.mPopertyData.end(); ++it)
            {
                PropertyChangedCallback_t localCallback = (*it).second->SetChangeCallback(boost::bind(&PropertySet::Notifier::onPropertyValueChanged, this, _1));
                mLocalPropertyCallbacks[(*it).first] = localCallback;
            }
        }

        wxASSERT(mExternalConnections.find(userSlotName) == mExternalConnections.end());
        mExternalConnections[userSlotName] = PropertyChangeEvent.connect(slot);
    }

    inline void PropertySet::Notifier::RemoveNotifySlot(const std::string& userSlotName, PropertySet& propertySet)
    {
        wxASSERT(mExternalConnections.find(userSlotName) != mExternalConnections.end());

        mExternalConnections[userSlotName].disconnect();
        mExternalConnections.erase(userSlotName);
        // if there is no more slots connected to signal, let's put back  local ones for each property
        if (mExternalConnections.empty())
        {
            for (PopertyData_t::iterator it = propertySet.mPopertyData.begin(); it != propertySet.mPopertyData.end(); ++it)
            {
                // extract stored local callback and set it back on property
                PropertyChangedCallback_t localCallback = mLocalPropertyCallbacks[(*it).first];
                (*it).second->SetChangeCallback(localCallback);
            }

            mLocalPropertyCallbacks.clear();
        }
    }

    inline void PropertySet::Notifier::onPropertyValueChanged(const IProperty *pProperty)
    {
        // first call original local callback associated with property,
        // then trigger signal
        PropertyCallbacks_t::iterator it = mLocalPropertyCallbacks.find(pProperty->GetName());
        wxASSERT(it != mLocalPropertyCallbacks.end());
        if ((*it).second)
        {
            (*it).second(pProperty);
        }

        PropertyChangeEvent(pProperty);
    }

    inline bool PropertySet::operator ==(const IPropertySet& source) const
    {
        // group names must be the same
        if (GetGroupName() != source.GetGroupName())
        {
            return false;
        }

        // if both objects do not have any properties they are the same
        if (!GetFirstProperty() && !source.GetFirstProperty())
        {
            return true;
        }

        if (const PropertySet *pSourcePropSet = dynamic_cast<const PropertySet *>(&source))
        {
            // we can speed this up a liltte bit by using PropertySet casted class
            if (mPopertyData.size() == pSourcePropSet->mPopertyData.size())
            {
                for (PopertyData_t::const_iterator it = mPopertyData.begin(); it != mPopertyData.end(); ++it)
                {
                    const IProperty *thisProp = (*it).second.get();
                    PopertyData_t::const_iterator it_source = pSourcePropSet->mPopertyData.find(thisProp->GetName());
                    if (it_source == pSourcePropSet->mPopertyData.end())
                    {
                        // this property does not exist in source
                        return false;
                    }

                    const IProperty *sourceProp = (*it_source).second.get();

                    if (!(thisProp->GetValue() == sourceProp->GetValue() &&
                        thisProp->GetPropType() == sourceProp->GetPropType() &&
                        thisProp->GetPropValue() == sourceProp->GetPropValue() &&
                        thisProp->GetName() == sourceProp->GetName()))
                    {
                        // one or more values are not the same
                        return false;
                    }
                }

                // both objects are the same
                return true;
            }
        }
        // slow method using interface
        IProperty *pProperty = GetFirstProperty();
        while (pProperty)
        {
            IProperty *pSourcePoperty = source.FindProperty(pProperty->GetName());
            if (!(pSourcePoperty &&
                pProperty->GetValue() == pSourcePoperty->GetValue() &&
                pProperty->GetPropType() == pSourcePoperty->GetPropType() &&
                pProperty->GetPropValue() == pSourcePoperty->GetPropValue() &&
                pProperty->GetName() == pSourcePoperty->GetName()))
            {
                // one or more values are not the same
                return false;
            }

            pProperty  = GetNextProperty();
        }

        // both objects are the same
        return true;
    }

    inline bool PropertySet::operator !=(const IPropertySet& source) const
    {
        return !((*this) == source);
    }


    namespace internal
    {
        struct PropertyTypesParser
        {
            typedef std::pair<uint32_t, uint32_t> PropType_t;
            enum eTypeValue
            {
                tvNone,
                tvBool,
                tvInt32,
                tvUint32,
                tvFloat,
                tvVector2,
                tvVector3,
                tvVector4,
                tvString,
                tvVectorString,
                tvFlags,
                tvColor3,
                tvColor4,
            };

            PropertyTypesParser(PropertySet& propertySet)
            : mPropertySet(propertySet)
            {

            }

            PropertyTypesParser(const PropertySet& propertySet)
            : mPropertySet(const_cast<PropertySet&>(propertySet))
            {

            }

            PropertySet& mPropertySet;

            PropType_t getTypeValue(IProperty *pProperty) const
            {
                PropType_t retValue(tvNone, tvNone);
                // what type this is
                if (pProperty->IsType<bool>())
                {
                    retValue.first = tvBool;
                }
                else if (pProperty->IsType<int32_t>())
                {
                    retValue.first = tvInt32;
                }
                else if (pProperty->IsType<uint32_t>())
                {
                    retValue.first = tvUint32;
                }
                else if (pProperty->IsType<float>())
                {
                    retValue.first = tvFloat;
                }
                else if (pProperty->IsType<Vector2>())
                {
                    retValue.first = tvVector2;
                }
                else if (pProperty->IsType<Vector3>())
                {
                    retValue.first = tvVector3;
                }
                else if (pProperty->IsType<Vector4>())
                {
                    retValue.first = tvVector4;
                }
                else if (pProperty->IsType<std::string>())
                {
                    retValue.first = tvString;
                }
                else if (pProperty->IsType<std::vector<std::string> >())
                {
                    retValue.first = tvVectorString;
                }
                else if (pProperty->IsType<Flags_Tag>())
                {
                    retValue.first = tvFlags;
                }
                else if (pProperty->IsType<Color3_Tag>())
                {
                    retValue.first = tvColor3;
                }
                else if (pProperty->IsType<Color4_Tag>())
                {
                    retValue.first = tvColor4;
                }
                else
                {
                    wxASSERT_MSG(0, "Unsupported IsType!");
                    wxLogError("Unsupported IsType: %s.", pProperty->GetPropType().name());
                }

                // what value this is
                if (pProperty->IsValue<bool>())
                {
                    retValue.second = tvBool;
                }
                else if (pProperty->IsValue<int32_t>())
                {
                    retValue.second = tvInt32;
                }
                else if (pProperty->IsValue<uint32_t>())
                {
                    retValue.second = tvUint32;
                }
                else if (pProperty->IsValue<float>())
                {
                    retValue.second = tvFloat;
                }
                else if (pProperty->IsValue<Vector2>())
                {
                    retValue.second = tvVector2;
                }
                else if (pProperty->IsValue<Vector3>())
                {
                    retValue.second = tvVector3;
                }
                else if (pProperty->IsValue<Vector4>())
                {
                    retValue.second = tvVector4;
                }
                else if (pProperty->IsValue<std::string>())
                {
                    retValue.second = tvString;
                }
                //else if (pProperty->IsValue<std::vector<std::string> >())
                //{
                //    retValue.second = tvVectorString;
                //}
                else
                {
                    wxASSERT_MSG(0, "Unsupported IsValue!");
                    wxLogError("Unsupported IsValue: %s.", pProperty->GetPropValue().name());
                }

                return retValue;
            }

            void addNewProperty(const PropType_t& propType , const std::string& propName)
            {
                if (propType.first == propType.second)
                {
                    // this will require slightly less code to process it
                    // then having both type and value different
                    switch (propType.first)
                    {
                        case tvBool:
                            mPropertySet.AddProperty<bool, bool>(propName, false);
                            break;

                        case tvInt32:
                            mPropertySet.AddProperty<int32_t, int32_t>(propName, 0);
                            break;

                        case tvUint32:
                            mPropertySet.AddProperty<uint32_t, uint32_t>(propName, 0);
                            break;

                        case tvFloat:
                            mPropertySet.AddProperty<float, float>(propName, 0);
                            break;

                        case tvVector2:
                            mPropertySet.AddProperty<Vector2, Vector2>(propName, v2::ZERO());
                            break;

                        case tvVector3:
                            mPropertySet.AddProperty<Vector3, Vector3>(propName, v3::ZERO());
                            break;

                        case tvVector4:
                            mPropertySet.AddProperty<Vector4, Vector4>(propName, v4::ZERO());
                            break;

                        case tvString:
                            mPropertySet.AddProperty<std::string, std::string>(propName, "");
                            break;

                        //case tvVectorString:
                        //    mPropertySet.AddProperty<std::vector<std::string>, std::vector<std::string> >(propName, std::vector<std::string>());
                        //    break;

                        default:
                            wxASSERT_MSG(0, "Unsupported addNewProperty with same Type and Value!");
                            wxLogError("Unsupported addNewProperty with same Type and Value!");

                    }
                }
                else if (propType.first != tvNone && propType.second != tvNone)
                {
                    if (propType.first == tvFlags && propType.second == tvUint32)
                    {
                        mPropertySet.AddProperty<Flags_Tag, uint32_t>(propName, 0);
                    }
                    else if (propType.first == tvColor3 && propType.second == tvVector3)
                    {
                        mPropertySet.AddProperty<Color3_Tag, Vector3>(propName, v3::ZERO());
                    }
                    else if (propType.first == tvColor4 && propType.second == tvVector4)
                    {
                        mPropertySet.AddProperty<Color4_Tag, Vector4>(propName, v4::ZERO());
                    }
                    else
                    {
                        wxASSERT_MSG(0, "Unsupported addNewProperty with different Type and Value!");
                        wxLogError("Unsupported addNewProperty with different Type and Value!");
                    }
                }
                else
                {
                    wxASSERT_MSG(0, "addNewProperty called with None for Type and Value!");
                    wxLogError("addNewProperty called with None for Type and Value!");
                }
            }

            //! look for BOOST_CLASS_VERSION below to see which version is used
            //friend class boost::serialization::access;

            template<class Archive>
            void save(Archive & ar, const unsigned int /*version*/) const
            {
                IProperty *pProperty = mPropertySet.GetFirstProperty();
                while (pProperty)
                {
                    // save Type and Value
                    PropType_t typeValue = getTypeValue(pProperty);
                    ar & typeValue.first;
                    ar & typeValue.second;
                    ar & pProperty->GetName();

                    pProperty = mPropertySet.GetNextProperty();
                }

                const uint32_t endMarker = 0;
                ar & endMarker;
                ar & endMarker;
            }

            template<class Archive>
            void load(Archive & ar, const unsigned int /*version*/)
            {
                PropType_t typeValue(tvNone, tvNone);
                ar & typeValue.first;
                ar & typeValue.second;
                while (typeValue.first != tvNone && typeValue.second != tvNone)
                {
                    std::string propName;
                    ar & propName;

                    // we got name and type of property let's create it
                    addNewProperty(typeValue, propName);

                    ar & typeValue.first;
                    ar & typeValue.second;
                }
            }

            BOOST_SERIALIZATION_SPLIT_MEMBER()

        private:
            //! no copy semantics
            PropertyTypesParser(const PropertyTypesParser&);
            PropertyTypesParser& operator=(const PropertyTypesParser&);
        };

    } // namespace internal

    template <typename AR>
    inline bool SaveProperySet(const PropertySet& propertySet, std::ostream& stream)
    {
        try
        {
            internal::PropertyTypesParser parser(propertySet);
            AR archive(stream);
            archive << parser;
            archive << propertySet;
        }
        catch (boost::archive::archive_exception& ex)
        {
            wxLogError("SaveProperySet failed with '%s' archive_exception.", ex.what());
            return false;
        }
        catch (std::exception& ex)
        {
            wxLogError("Exception '%s' in SaveProperySet", ex.what());
            return false;
        }

        return true;
    }

    template <typename AR>
    inline bool LoadPropertySet(PropertySet& propertySet, std::istream& stream)
    {
        try
        {
            internal::PropertyTypesParser parser(propertySet);
            AR archive(stream);
            archive >> parser;
            archive >> propertySet;
        }
        catch (boost::archive::archive_exception& ex)
        {
            wxLogError("LoadPropertySet failed with '%s' archive_exception.", ex.what());
            return false;
        }
        catch (std::exception& ex)
        {
            wxLogError("Exception '%s' in LoadPropertySet", ex.what());
            return false;
        }

        return true;
    }

    // ------------------------------------------------------------------------------------------------
    // PropertySet End (PropertySet : public IPropertySet)
// ------------------------------------------------------------------------------------------------

} // namespace eg

BOOST_CLASS_VERSION(eg::internal::PropertyTypesParser, 1);
BOOST_CLASS_TRACKING(eg::internal::PropertyTypesParser, boost::serialization::track_never);

#endif // PROPERTY_PROPERTY_DATA_IMPL_H

