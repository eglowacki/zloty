///////////////////////////////////////////////////////////////////////
// PropertyData.h
//
//  Copyright 8/8/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Classes used to provide property for components
//
//
//  #include "Property/PropertyData.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef PROPERTY_PROPERTY_DATA_H
#define PROPERTY_PROPERTY_DATA_H
#pragma once

#include "Property/IProperty.h"
#include "PropertyDataConversion.h"
#include <typeinfo>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/signal.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>

namespace eg
{
    class PropertySet;

    class PropertySetSignal : public IPropertySet
    {
    public:
        typedef boost::signal<void (const IProperty * /*pProperty*/)> NotifySignal_t;
    };


    /*!
    Concrete implementation IProperty interface as template for what kind of value this property represents
    */
    template <typename T>
    class Property : public IProperty
    {
    public:
        Property(PropertySet *pPropertySet, const std::string& name, T *value, bool readOnly, PropertyChangedCallback_t propertyChangedCallback, const std::string& typeOptions);
        virtual ~Property();

        // from Property
        virtual uint32_t GetPropId() const;
        virtual const std::string& GetName() const;
        virtual std::string GetValue() const;
        virtual void SetValue(const std::string& newValue);
        virtual bool IsReadOnly() const;
        virtual const std::type_info& GetPropType() const;
        virtual const std::string& GetPropTypeOptions() const;
        virtual const std::type_info& GetPropValue() const;
        virtual component::InstanceId GetInstanceId() const;
        virtual PropertyChangedCallback_t SetChangeCallback(PropertyChangedCallback_t propertyChangedCallback);
        //! Assignment operators for type T
        Property& operator =(const T& value);

        //! Conversion operator to type T
        operator T() const;

        // \note why do we have this?
        const T& operator()() const;
        T& operator()();

    private:
        std::string mName;
        T *mValue;
        bool mReadOnly;
        PropertySet *mpPropertySet;
        PropertyChangedCallback_t mPropertyChangedCallback;
        uint32_t mPropId;
        std::string mPropTypeOptions;

        virtual void SetNativeValue(const T& value);
    };

    template <typename T>
    class BlankProperty : public Property<T>
    {
    public:
        BlankProperty()
        : Property(0, "Blank", &mBlankValue, true, PropertyChangedCallback_t())
        , mBlankValue(T())
        {}

    private:
        T mBlankValue;
    };


    /*!
     Generic property which provides type and value
     where Type can be any type of special
     Tag structures (at the top of this file)
     and Value id the actual value representing Type
     */
    template <typename T, typename V>
    class PropertyGeneric : public Property<V>
    {
    public:
        PropertyGeneric(PropertySet *pPropertySet, const std::string& name, V *value, bool readOnly, PropertyChangedCallback_t propertyChangedCallback, const std::string& typeOptions);
        virtual ~PropertyGeneric();
        virtual const std::type_info& GetPropType() const;
        virtual const std::type_info& GetPropValue() const;

        //! Assignment operators for type T
        PropertyGeneric& operator =(const V& value);
        const PropertyGeneric& operator =(const V& value) const;
        //! Conversion operator to type T
        operator V() const;
    };


    /*!
     Self seeled property which contains it's own value
     */
    template <typename T, typename V>
    class PropertyOwn : public PropertyGeneric<T, V>
    {
    public:
        PropertyOwn(PropertySet *pPropertySet, const std::string& name, const V& value, PropertyChangedCallback_t propertyChangedCallback, const std::string& typeOptions)
        : PropertyGeneric(pPropertySet, name, &mValue, false, propertyChangedCallback, typeOptions)
        , mValue(value)
        {
        }

        //! Assignment operators for type T
        PropertyOwn& operator =(const V& value);
        const PropertyOwn& operator =(const V& value) const;
        //! Conversion operator to type T
        operator V() const;

    private:
         V mValue;
    };


    /*!
    Concrete implementation of IPropertySet
    */
    class PropertySet : public PropertySetSignal
    {
    public:
        PropertySet();

        //! This will provide this call back to all of the created properties
        //! \code
        //!     // provided as a parameter to this ctor
        //!     boost::bind(&ClassFoo::MyCallback, &myFoo, _1)
        //! \endcode
        PropertySet(const component::InstanceId& instanceId, const std::string& groupName, PropertyChangedCallback_t propertyChangedCallback);
        virtual ~PropertySet();
        virtual bool IsValid() const;
        virtual const std::string& GetGroupName() const;
        virtual IProperty *GetFirstProperty() const;
        virtual IProperty *GetNextProperty() const;
        virtual IProperty *FindProperty(const std::string& propName) const;
        virtual component::InstanceId GetInstanceId() const;
        virtual void SetGroupName(const std::string& groupName);
        virtual void AddNotifySlot(const IPropertySet::NotifySlot_t& slot, const std::string& userSlotName);
        virtual void RemoveNotifySlot(const std::string& userSlotName);
        virtual bool operator ==(const IPropertySet& source) const;
        virtual bool operator !=(const IPropertySet& source) const;
        // Index operator which will return reference to IProperty object. This assumes that property is already created
        virtual IProperty& operator[](const std::string& propName);
        virtual const IProperty& operator[](const std::string& propName) const;

        /*!
        This will create property which will expose certain type to editor and intended value for run time
        \code
            Vector4 clearSurfaceColor(1, 1, 1, 1);
            PropertySet propertySet;
            // ...
            propertySet.AddProperty<Color4_Tag, Vector4>("Clear Color", &clearSurfaceColor, false);
        \endcode
        */
        template <typename T, typename V>
        void AddProperty(const std::string name, V *value, bool readOnly, PropertyChangedCallback_t propertyChangedCallback = PropertyChangedCallback_t(), const std::string& typeOptions = "");

        /*!
        This add self contain property value
        \code
            PropertySet propertySet;
            // ...
            propertySet.AddProperty<Color4_Tag, Vector4>("Clear Color", v4::One());
        \endcode
        */
        template <typename T, typename V>
        void AddProperty(const std::string name, const V& value, PropertyChangedCallback_t propertyChangedCallback = PropertyChangedCallback_t(), const std::string& typeOptions = "");

    private:
        typedef std::map<std::string, boost::shared_ptr<IProperty> > PopertyData_t;
        mutable PopertyData_t mPopertyData;
        mutable PopertyData_t::iterator mProp_it;
        std::string mGroupName;
        component::InstanceId mInstanceId;
        PropertyChangedCallback_t mPropertyChangedCallback;

        //! This handles global signal when there is a change to any of property values
        //! This is used mostly by editor functionality and is trying to minimize performance
        //! issues.
        //! In dormant state (where there is no global signal hooked up), the only
        //! data overhead will be two empty maps and empty boost signal
        //! We are wrapping this into it's own structure
        struct Notifier
        {
            void onPropertyValueChanged(const IProperty *pProperty);

            void AddNotifySlot(const IPropertySet::NotifySlot_t& slot, const std::string& userSlotName, PropertySet& propertySet);
            void RemoveNotifySlot(const std::string& userSlotName, PropertySet& propertySet);

            //! Local property callbacks set on Property
            //! first - property name
            //! second - local callback
            typedef std::map<std::string, PropertyChangedCallback_t> PropertyCallbacks_t;
            PropertyCallbacks_t mLocalPropertyCallbacks;
            PropertySetSignal::NotifySignal_t PropertyChangeEvent;

            //! Keeps track of current outside connections hooked to properties
            //! first - unique name of this connection provided by user
            //! second - connection to signal. When this is destroyed connection is lost
            typedef std::map<std::string, boost::signals::connection> ExternalConnections_t;
            ExternalConnections_t mExternalConnections;
        };

        Notifier mNotifier;
    };

    //! Save entire property set including types of each property.
    //! This is useful when we want to load property data back and construct
    //! all of properties at run time from stream.
    //!
    //! \param propertySet data set to save
    //! \param stream where to save it
    //! \param typename AR can be boost::archive::text_oarchive
    //!
    //! \return bool all is OK and was able to save data into stream, otherwise FALSE
    template <typename AR>
    bool SaveProperySet(const PropertySet& propertySet, std::ostream& stream);

    //! Load entire property data set from stream constructing all properties as needed
    //!
    //! \param propertySet load this data from stream
    //! \param stream stream to load it from
    //! \param typename AR can be boost::archive::text_iarchive
    //!
    //! \return bool all was OK and was able to load all the data into propertySet, otherwise FALSE
    template <typename AR>
    bool LoadPropertySet(PropertySet& propertySet, std::istream& stream);

    /*!
    Some samples of usage for get_prop(V) functions

    \code
    class TestPropertyHolder
    {
    public:
        TestPropertyHolder()
        : mPropertySet(boost::bind(&TestPropertyHolder::onPropertyChanged, this, _1))
        , mState(get_propV<int32_t>(mPropertySet, "State", 5))
        , mColor(get_prop<Color3_Tag, Vector3>(mPropertySet, "Color", v3::ONE()))

    private:
        void onPropertyChanged(const IProperty *pProperty)
        {
            // you can compare pProperty:
            if (pProperty->GetPropId() == mState.GetPropId())
            {
                // state was changed
            }
        }

        // to use memeber initialization list corectly,
        // make sure that PropertySet is first in member list
        PropertySet mPropertySet;
        Property<int32_t>& mState;
        Property<Vector3>& mColor;
    };

    // usage of property stuff (setters and getters)
    uint32_t stateValue = mState;
    // this will generate onPropertyChanged event
    mState = stateValue;

    // PropertySet container work almost the same way but it's using
    // string as a setter and getter values
    mPropertySet["State"] = "47";
    std::string stateValue = mPropertySet["State"];
    \endcode

    */
    //! Return Property cast to specific type from property set and name of property
    //! This assumes that property is already created and is of type T
    template <typename T>
    Property<T>& prop_cast(IPropertySet& propSet, const std::string& propName)
    {
        IProperty& iProp = propSet[propName];
        Property<T>& prop = dynamic_cast<Property<T>& >(iProp);
        return prop;
    }


    //! Return property cast it to specific type. If property does not exist, create.
    template <typename T, typename V>
    Property<V>& get_prop(PropertySet& propSet, const std::string& propName, const V& value, const std::string& typeOptions)
    {
        if (!propSet.FindProperty(propName))
        {
            propSet.AddProperty<T, V>(propName, value, PropertyChangedCallback_t(), typeOptions);
        }

        return prop_cast<V>(propSet, propName);
    }


    template <typename V>
    Property<V>& get_propV(PropertySet& propSet, const std::string& propName, const V& value, const std::string& typeOptions)
    {
        return get_prop<V, V>(propSet, propName, value, typeOptions);
    }
} // namespace eg

BOOST_CLASS_VERSION(eg::IProperty, 1)
BOOST_CLASS_VERSION(eg::IPropertySet, 1)

#define PROPERTY_PROPERTY_DATA_INCLUDE_IMPLEMENTATION
#include "PropertyDataImpl.h"
#undef PROPERTY_PROPERTY_DATA_INCLUDE_IMPLEMENTATION


#endif // PROPERTY_PROPERTY_DATA_H

