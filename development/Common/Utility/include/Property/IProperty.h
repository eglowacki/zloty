///////////////////////////////////////////////////////////////////////
// IProperty.h
//
//  Copyright 10/20/2008 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Classes used to provide property interface only for components
//
//
//  #include "Property/IProperty.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef PROPERTY_IPROPERTY_H
#define PROPERTY_IPROPERTY_H
#pragma once

#include "Base.h"
#include "PropertyDataConversion.h"
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>


namespace eg
{
	class IProperty;

	//! Types of property we provide, besides the built in types
	//! represented by Vector3 and it provides color picker
	struct Color3_Tag {};

	//! represented by Vector4 and it provides color picker (alpha is still not exposed)
	struct Color4_Tag {};

	//! represented by std::string and provides file open dialog
	//! This will send kGetResourcePath, where PropertyInfo_t is data
	//! Second message is kGetResourceFilter, where PropertyInfo_t is data
	struct File_Tag {};

	//! represented by std::string and provides list box of available materials
	struct Material_Tag {};

	//! represented by std::string and provides list box of available render targets
	struct RenderTarget_Tag {};

	//! represented by uint32_t and provides bits to selected
	//! This will send kGetChoiceLabels, where std::pair<std::string, std::vector<std::string> > is data
	struct Flags_Tag {};

	//! choice from pre-defined string
	//! This will send kGetChoiceLabels, where std::pair<std::string, std::vector<std::string> > is data
	struct Choice_Tag {};

	//! This is used to be able provide custom label(s)
	//! This will send kGetChoiceLabels, where std::pair<std::string, std::vector<std::string> > is data
	struct CustomLabels_Tag {};

	//! This is used to show enum values by it's name
	//! This will send kGetChoiceLabels, where std::pair<std::string, std::vector<std::string> > is data
	//! .first - name of property (read only)
	//! .second - recipient of this message fills in names of enums
	struct Enum_Tag {};


	//! This is used to be able to receive callbacks when property values is changed
	//!
	//! \code
	//!     PropertyChangedCallback_t propertyChangedCallback = boost::bind(&ClassFoo::MyCallback, &myFoo, _1);
	//! \endcode
	typedef boost::function<void (const IProperty * /*pProperty*/)> PropertyChangedCallback_t;


	//! As much as I don't like macros, this does come handy here, since there is
	//! quite bit of boiler plate code for handling native types in assignment operators
	//! The base implementation uses ConvertProperty(...) functions which take std::string
	//! and convert to valueType or valueType to std::string.
	//! You can override any of SetNativeValue() and/or GetNativeValue()
	//! method in derive class to handle native values
	//! The only time any of this methods are called
	//! if this instance will return true for IsValue<type>().
	//! Also in this implementation if Property is ReadOnly it will not set it,
	//! but in your override, it's up to you to handle that.
	#define YAGET_PROP_ASSIGNMENT(valueType) \
	public: \
		IProperty& operator =(const valueType& value) \
		{ \
		if (IsValue<valueType>()) \
		{ \
		SetNativeValue(value); \
		} \
		return *this; \
		} \
		operator valueType() const \
		{ \
		return GetNativeValue(valueType()); \
		} \
	private: \
		virtual void SetNativeValue(const valueType& value) \
		{ \
		*this = ConvertProperty(value); \
		} \
		/* We have to pass unused parameter, as to allow distinction between virtual methods */ \
		virtual valueType GetNativeValue(const valueType&) const \
		{ \
		return ConvertProperty<valueType>(GetValue()); \
		} \
	public:

		//! This can be used on one of derived classes from IProperty to
		//! provide native support to set value directly, without
		//! converting 'value -> string -> value'.
	#define YAGET_NATIVE_PROP_ASSIGNMENT(valueType) \
	private: \
		virtual void SetNativeValue(const valueType& value) \
		{ \
		*this = value; \
		} \
	public:


	/*!
	This represents one property data. All manipulation is done through std::string.
	There are two kinds of property types, one just has Value
	and the other can have type and value, where Type is used by editors to
	provide and query appropriate editing controls.

	It also provided serialization routines
	Saving property values to a file:
	\code
	const eg::IPropertySet &somePropSet = <sone_property_set>;

	std::ofstream ofs("PropertyData.dat");
	boost::archive::text_oarchive archive(ofs);
	archive << somePropSet;
	\endcode

	Loading property values from a file:
	\code
	eg::IPropertySet &somePropSet = <sone_property_set>;

	std::ifstream ifs("PropertyData.dat", std::ios::binary);
	boost::archive::text_iarchive archive(ifs);
	archive >> somePropSet;
	\endcode
	*/
	class IProperty
	{
	public:
		virtual ~IProperty() = 0
		{}

		//! This will return unique id for this property within property set
		virtual uint32_t GetPropId() const = 0;

		//! return name of this property.
		virtual const std::string& GetName() const = 0;

		//! Return current value of this property in a form of std::string. Conversion might occur from type T to std::string
		virtual std::string GetValue() const = 0;

		//! Set property value to newValue. Conversion might occur from std::string to type T
		//! \note It will silently ignore newValue if this is ReadOnly property.
		virtual void SetValue(const std::string& newValue) = 0;

		//! Is this property read only, then return TRUE, otherwise FALSE
		virtual bool IsReadOnly() const = 0;

		//! Get what kind of type this value exposes
		//! This and GetPropValue() methods are used to drive editing capabilities
		//! for this property. IOW, it allows for this property to expose customized
		//! edit controls to editor.
		//!
		//! \note Type, can be any built-ins or one of 'struct *_Tag {};' structure's
		virtual const std::type_info& GetPropType() const = 0;

		//! Based on what Type this property represents
		//! there might be some extra data needed for edit control or editor
		//! ex: for File_Tag options will be: {ext = "*.tga", type = "tga"}
		virtual const std::string& GetPropTypeOptions() const = 0;

		//! Get what kind of value this property is
		virtual const std::type_info& GetPropValue() const = 0;

		//! Return instance id of owner of this property
		virtual component::InstanceId GetInstanceId() const = 0;


		//! This is primary used by editor to be notified when there is a change to property value.
		//! \note Do not call this directly, it is used by PropertySet to implement that functionality
		//! \param propertyChangedCallback new callback to use
		//! \return previous callback used or PropertyChangedCallback_t() if none
		virtual PropertyChangedCallback_t SetChangeCallback(PropertyChangedCallback_t propertyChangedCallback) = 0;

		IProperty& operator =(const char *pvalue) { return (*this) = std::string(pvalue);}

		IProperty& operator =(const std::string& value);
		operator std::string() const;

		//!@{
		//! We provide certain set of values which can be use to set this IProperty
		//! to that value.
		//! For comments about overriding virtual get and set methods lookup
		//! YAGET_PROP_ASSIGNMENT macro
		YAGET_PROP_ASSIGNMENT(bool);
		YAGET_PROP_ASSIGNMENT(float);
		YAGET_PROP_ASSIGNMENT(int32_t);
		YAGET_PROP_ASSIGNMENT(uint32_t);
		YAGET_PROP_ASSIGNMENT(uint64_t);
		YAGET_PROP_ASSIGNMENT(Vector2);
		YAGET_PROP_ASSIGNMENT(Vector3);
		YAGET_PROP_ASSIGNMENT(Vector4);
		YAGET_PROP_ASSIGNMENT(Quaternion);
		YAGET_PROP_ASSIGNMENT(Matrix33);
		YAGET_PROP_ASSIGNMENT(Matrix44);
		YAGET_PROP_ASSIGNMENT(std::vector<component::ObjectId>);
		YAGET_PROP_ASSIGNMENT(std::vector<std::string>);
		//!@}


		//!@{
		//! Helper method for checking type and value
		template <typename T>
		bool IsType() const {return (typeid(T) == GetPropType()) ? true : false;}
		template <typename T>
		bool IsValue() const {return (typeid(T) == GetPropValue()) ? true : false;}
		//!@}

	private:
		//! look for BOOST_CLASS_VERSION below to see which version is used
		friend class boost::serialization::access;

		template<class Archive>
		void save(Archive & ar, const unsigned int version) const;
		template<class Archive>
		void load(Archive & ar, const unsigned int version);

		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};


	//----------------------------------------------------------------------------
	/*!
	For now, this class only purpose is to provide access to
	editor panels for component data
	This class provides collection of Properties
	*/
	class IPropertySet
	{
	public:
		virtual ~IPropertySet() = 0
		{}
		//! Is this property set is valid, or just maybe placeholder
		virtual bool IsValid() const = 0;

		//! Get group name for this set of properties
		virtual const std::string& GetGroupName() const = 0;

		virtual void SetGroupName(const std::string& groupName) = 0;

		//! Returns first property or null if none exist. Needs to be called first before using GetNextProperty(...)
		virtual IProperty *GetFirstProperty() const = 0;

		//! Returns next iterated property, or NULL if no more. GetFirstProperty(...) must be called before using this.
		virtual IProperty *GetNextProperty() const = 0;

		//! Find property with specific name and return it, or NULL if does not exists
		virtual IProperty *FindProperty(const std::string& propName) const = 0;

		//! Set value on specific property. Return TRUE if was set, otherwise FALSE (property did not exist)
		virtual bool SetProperty(const std::string& propName, const std::string& newValue);

		//! Set instance id of owner of this property set. Each Property will also inherit this instance id.
		//virtual void SetInstanceId(ComponentInstanceId_t instanceId) = 0;

		//! get instance id of owner of this property set
		virtual component::InstanceId GetInstanceId() const = 0;

		//! This allows us to provide callback which will be triggered
		//! everytime there is change in value of any property.
		//! This is useful in editor to be able to display changing values.
		//! Signature of callback slot for global property change event
		typedef boost::function<void (const IProperty * /*pProperty*/)> NotifySlot_t; 

		//! This will add new notify slot
		//!
		//! \param slot callback function to be called when any of properties change
		//! \param userSlotName user define name. This name must be unique across all callbacks connected to this property set
		//! \note if in debug mode it will assert and in release it will silently replace previous callback.
		//! \code boost::bind(&Foo::onSignal, this, _1) \endcode
		virtual void AddNotifySlot(const NotifySlot_t& slot, const std::string& userSlotName) = 0;

		//! This will remove existing notify slot
		//!
		//! \param userSlotName slot to remove
		virtual void RemoveNotifySlot(const std::string& userSlotName) = 0;

		virtual bool operator ==(const IPropertySet& source) const = 0;
		virtual bool operator !=(const IPropertySet& source) const = 0;
		virtual IProperty& operator[](const std::string& propName) = 0;
		virtual const IProperty& operator[](const std::string& propName) const = 0;

	private:
		//! look for BOOST_CLASS_VERSION below to see which version is used
		friend class boost::serialization::access;

		template<class Archive>
		void save(Archive & ar, const unsigned int version) const;
		template<class Archive>
		void load(Archive & ar, const unsigned int version);

		BOOST_SERIALIZATION_SPLIT_MEMBER()
	};


	// ------------------------------------------------------------------------------------------------
	// IProperty Begin
	// ------------------------------------------------------------------------------------------------
	inline IProperty& IProperty::operator =(const std::string& value)
	{
		SetValue(value);
		return *this;
	}

	inline IProperty::operator std::string() const
	{
		return GetValue();
	}

	template<class Archive>
	inline void IProperty::save(Archive & ar, const unsigned int /*version*/) const
	{
		std::string currValue = GetValue().c_str();
		ar & currValue;
	}

	template<class Archive>
	inline void IProperty::load(Archive & ar, const unsigned int /*version*/)
	{
		std::string newValue;
		ar & newValue;
		SetValue(newValue);
	}
	// ------------------------------------------------------------------------------------------------
	// IProperty End
	// ------------------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------------------
	// IPropertySet Begin
	// ------------------------------------------------------------------------------------------------
	inline bool IPropertySet::SetProperty(const std::string& propName, const std::string& newValue)
	{
		if (IProperty *pProperty = FindProperty(propName))
		{
			pProperty->SetValue(newValue);
			return true;
		}

		return false;
	}


	template<class Archive>
	inline void IPropertySet::save(Archive & ar, const unsigned int /*version*/) const
	{
		const std::string& groupName = GetGroupName();
		ar & groupName;

		IProperty *pProperty = GetFirstProperty();
		while (pProperty)
		{
			ar & (*pProperty);
			pProperty = GetNextProperty();
		}
	}

	template<class Archive>
	inline void IPropertySet::load(Archive & ar, const unsigned int /*version*/)
	{
		std::string groupName;
		ar & groupName;
		SetGroupName(groupName.c_str());
		// \note we need to have code to match property by saved name of that property
		// and not assume that order will be preserved between save and load
		IProperty *pProperty = GetFirstProperty();
		while (pProperty)
		{
			ar & (*pProperty);
			pProperty = GetNextProperty();
		}
	}
	// ------------------------------------------------------------------------------------------------
	// IPropertySet End
	// ------------------------------------------------------------------------------------------------


} // namespace eg
#endif // PROPERTY_IPROPERTY_H