/////////////////////////////////////////////////////////////////////
// WidgetAsset.h
//
//  Copyright 2/8/2008 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Base class used on derivation of Widgets
//      which are helpers and editing gizmos
//      It can render wire frame, bounding boxes, diffrent
//      higlights, etc.
//
//
//  #include "Widget/WidgetAsset.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef WIDGET_WIDGET_ASSET_H
#define WIDGET_WIDGET_ASSET_H
#pragma once

#pragma warning(push)
#pragma warning (disable : 4267) // var' : conversion from 'size_t' to 'type', possible loss of data
    #include <boost/archive/binary_oarchive.hpp>
    #include <boost/archive/binary_iarchive.hpp>
#pragma warning(pop)

#include "ObjectID.h"
#include "Asset/Asset.h"
#include <boost/tokenizer.hpp>

namespace eg
{
    template <typename T>
    class WidgetAsset : public eg::Asset<T>
    {
    public:
        static const guid_t kType = 0x34b8acfa;

        WidgetAsset(const std::string& name)
        : eg::Asset<T>(name)
        , mShowWidget(true)
        , mObjectId()
        , mRenderId(1)
        {
            Dispatcher& disp = REGISTRATE(Dispatcher);
            mRenderEndConnection = disp[mtype::kEndRenderFrame].connect(0, boost::bind(&WidgetAsset<T>::onEndRender, this, _1));
        }

        virtual ~WidgetAsset()
        {
            mRenderEndConnection.disconnect();
        }

        void SetObjectId(const component::ObjectId& oId) {mObjectId = oId;}
        void Show() {mShowWidget = true;}
        void Hide() {mShowWidget = false;}
        //! Specific options for individual widgets,
        //! in a form of key=value pair, colon delimited
        void SetOptions(const std::string& options)
        {
            assert(0);
            /*
            // split by comma
            wxStringTokenizer tokenizer(options, wxT(","));
            while (tokenizer.HasMoreTokens())
            {
                // this is 'key + "=" + value' format
                wxString token = tokenizer.GetNextToken();

                //std::string objectIdString;
                //std::string compTypeString;
                wxStringTokenizer pairKVP(token, wxT("="));
                if (pairKVP.CountTokens() == 2)
                {
                    std::string key = pairKVP.GetNextToken();
                    std::string value = pairKVP.GetNextToken();

                    setOption(key, value);
                }
            }
            */
        }

        virtual void HandleMessage(Message& msg)
        {
            if (msg.Is<std::string>())
            {
                std::string value = msg.GetValue<std::string>();

                if (boost::algorithm::iequals(value, std::string("QueryOptions")))
                {
                    // we need to get all options possible for this widget and return
                    std::string options;
                    getOptions(options);
                    options = Name() + " {" + options + "}";
                    msg.mAnyData = options;
                }
                else
                {
                    wxStringTokenizer tokenizer(value, wxT(";"));
                    while (tokenizer.HasMoreTokens())
                    {
                        // this is 'some text' + "," + 'some more text' format
                        std::string kvPair = tokenizer.GetNextToken();
                        // now split this by '='
                        wxStringTokenizer kvToken(kvPair, wxT("="));
                        if (kvToken.CountTokens() == 2)
                        {
                            std::string key = kvToken.GetNextToken();
                            std::string value = kvToken.GetNextToken();
                            boost::trim(key);
                            boost::trim(value);

                            if (boost::algorithm::iequals(key, std::string("objectid")))
                            {
                                try
                                {
                                    uint32_t id = boost::lexical_cast<uint32_t>(value);
                                    SetObjectId(component::ObjectId(id));
                                }
                                catch (boost::bad_lexical_cast&)
                                {
                                    // do nothing here but we still want to log the error here
                                    wxLogError("Could not convert WidgetAsset message from string to string, where key [%s], value [%s] pair.", key.c_str(), value.c_str());
                                }
                            }
                            else if (boost::algorithm::iequals(key, std::string("show")))
                            {
                                bool bShow = ConvertProperty<bool>(value);
                                if (bShow)
                                {
                                    Show();
                                }
                                else
                                {
                                    Hide();
                                }
                            }
                            else if (boost::algorithm::iequals(key, std::string("order")))
                            {
                                int order = ConvertProperty<int>(value);
                                mRenderEndConnection.disconnect();
                                Dispatcher& disp = REGISTRATE(Dispatcher);
                                mRenderEndConnection = disp[mtype::kEndRenderFrame].connect(order, boost::bind(&WidgetAsset<T>::onEndRender, this, _1));
                            }
                            if (boost::algorithm::iequals(key, std::string("renderid")))
                            {
                                mRenderId = ConvertProperty<uint32_t>(value);
                            }
                            else
                            {
                                setOption(key, value);
                            }
                        }
                    }
                }
            }
        }

    protected:
        bool showWidget() const {return mShowWidget;}
        const component::ObjectId& oId() const {return mObjectId;}
        //! Return all available options for this widget
        //! derived classes should call this one before adding it's own options
        virtual void getOptions(std::string& options) const
        {
            if (!options.empty())
            {
                options += "; ";
            }
            options += "objectid = <id>; show = <true|false>; order = <int>; renderid = <uint>";
        }

        uint32_t renderId() const {return mRenderId;}

    private:
        //! Called for each key/value pair extracted from options string
        virtual void setOption(const std::string& key, const std::string& value) = 0;
        virtual void onEndRender(eg::Message& /*msg*/) {}

        uint32_t mRenderId;
        //! flag to specify if this widget should be visible or not
        bool mShowWidget;
        component::ObjectId mObjectId;
        boost::signals::connection mRenderEndConnection;
        //-------------------------------------------------------------------------------------
        //! serialization block
        //! look for BOOST_CLASS_VERSION below to see which version is used
        friend class boost::serialization::access;

        // Split version of serialize methods
        template<class Archive>
        void save(Archive & ar, const unsigned int /*version*/) const
        {
            //DataLocker lock(*this);
            ar & mShowWidget;
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int /*version*/)
        {
            //DataLocker lock(*this);
            ar & mShowWidget;
        }

        BOOST_SERIALIZATION_SPLIT_MEMBER()
        //-------------------------------------------------------------------------------------
    };


    namespace widgets
    {
         YAGET_BASE_SYMBOL guid_t GetWidgetType(const std::string& name);

         YAGET_BASE_SYMBOL void AddWidgetType(const std::string& name, guid_t type);
         YAGET_BASE_SYMBOL void RemoveWidgetType(const std::string& name);

         /*!
         Use this to automatically register widget by name.
         \code
         AutoRegisterAssetFactory<MyWidget> myFactory;
         widgets::AutoRegister<MyWidget> registerWidget("MyWidget");
         \endcode
         */
         template <typename T>
         struct AutoRegister
         {
             AutoRegister(const std::string& name)
             {
                 AddWidgetType(name, T::kType);
             }
         };


    } // namespace widgets

} // namespace eg


#endif // WIDGET_WIDGET_ASSET_H

