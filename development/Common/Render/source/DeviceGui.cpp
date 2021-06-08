#include "Device.h"
//#include "imgui.h"
#include "Gui/Support.h"

void yaget::render::Device::Gui_UpdateWatcher()
{
    //static bool watcherWindow = false;
    //static bool frameResourceWindow = false;
    //static bool aboutWindow = false;

    //yaget::gui::Fonter fonter("Consola");
    //if (ImGui::BeginMainMenuBar())
    //{
    //    if (ImGui::BeginMenu("Device"))
    //    {
    //        ImGui::MenuItem("Watcher Window", "", &watcherWindow);
    //        ImGui::MenuItem("Frame Resource Window", "", &frameResourceWindow);

    //        ImGui::EndMenu();
    //    }

    //    ImGui::EndMainMenuBar();
    //}

    //if (watcherWindow)
    //{
    //    yaget::Strings watchedFiles = mResourceWatcher.GetWatchedFiles();
    //    ImGui::Begin("Resource Watcher", &watcherWindow);

    //    for (const auto& fileName : watchedFiles)
    //    {
    //        ImGui::Text(fileName.c_str());
    //    }

    //    ImGui::End();
    //}

    //if (frameResourceWindow)
    //{
    //    using Section = io::VirtualTransportSystem::Section;
    //    auto keyColor = dev::CurrentConfiguration().mGuiColors.at("KeyText");
    //    auto valueColor = dev::CurrentConfiguration().mGuiColors.at("ValueText");
    //    auto activeColor = dev::CurrentConfiguration().mGuiColors.at("ActiveText");
    //    auto sectionColor = dev::CurrentConfiguration().mGuiColors.at("SectionText");
    //    auto infoColor = dev::CurrentConfiguration().mGuiColors.at("InfoText");

    //    ImGui::Begin("Frame Resource Activation", &frameResourceWindow, ImGuiWindowFlags_MenuBar);

    //    if (ImGui::BeginMenuBar())
    //    {
    //        if (ImGui::BeginMenu("Help"))
    //        {
    //            ImGui::MenuItem("About", "", &aboutWindow);
    //            ImGui::EndMenu();
    //        }

    //        ImGui::EndMenuBar();
    //    }

    //    if (aboutWindow)
    //    {
    //        ImGui::Begin("About", &aboutWindow);
    //        ImGui::Text("Device frame resource stack.");
    //        ImGui::End();
    //    }

    //    bool debuggerAttached = platform::IsDebuggerAttached();

    //    std::string typeFilter, hashFilter, nameFilter;

    //    std::vector<int> highlights;
    //    int index = 0;
    //    std::size_t valueTextLength = 0;
    //    std::size_t userTextLength = 0;
    //    std::for_each(std::begin(mFrameResources), std::end(mFrameResources), [&/*this, &valueTextLength, &userTextLength, &index, &highlights*/](auto& resource)
    //    {
    //        if (resource.mResource)
    //        {
    //            valueTextLength = std::max(valueTextLength, std::strlen(resource.mResource->GetNameType()));

    //            if (!resource.mUserText.empty())
    //            {
    //                userTextLength = std::max(userTextLength, std::string(" (" + resource.mUserText + ")").length());
    //            }

    //            if (index < mPreviousFrameResources.size() && mPreviousFrameResources[index].mResource == resource.mResource)
    //            {
    //                resource.mHighlightType = mPreviousFrameResources[index].mHighlightType;
    //                resource.mHighlightHash = mPreviousFrameResources[index].mHighlightHash;
    //                resource.mHighlightName = mPreviousFrameResources[index].mHighlightName;
    //            }

    //            if (resource.mHighlightType || resource.mHighlightHash || resource.mHighlightName)
    //            {
    //                highlights.push_back(index);
    //            }
    //        }

    //        ++index;
    //    });

    //    int highlightedIndex = -1;
    //    if (!highlights.empty())
    //    {
    //        highlightedIndex = *highlights.begin();

    //        typeFilter = mFrameResources[highlightedIndex].mHighlightType ? mFrameResources[highlightedIndex].mResource->GetNameType() : "";
    //        hashFilter = mFrameResources[highlightedIndex].mHighlightHash ? fmt::format("{}", mFrameResources[highlightedIndex].mResource->GetStateHash()) : "";
    //        nameFilter = mFrameResources[highlightedIndex].mHighlightName ? Section(mFrameResources[highlightedIndex].mResource->mAssetTag).ToString() : "";
    //    }

    //    index = 0;
    //    const std::string ValueFormater("{:" + std::to_string(valueTextLength) + "}");
    //    const std::string UserFormater("{:" + std::to_string(userTextLength) + "}");

    //    float defaultAlphaAdjuster = highlights.empty() ? 1.0f : 0.6f;
    //    for (auto& resource : mFrameResources)
    //    {
    //        if (resource.mResource)
    //        {
    //            if (debuggerAttached)
    //            {
    //                if (ImGui::Button(fmt::format("Debug##{}", index).c_str()))
    //                {
    //                    mResourceBreakOn = index;
    //                }
    //                yaget::gui::SetTooltip("Debug-Break when Activating this Resource");
    //                ImGui::SameLine();
    //            }

    //            yaget::gui::MakeDisabled(highlightedIndex != -1 && highlightedIndex != index, [&]()
    //            {
    //                ImGui::Checkbox(fmt::format("T##{}", index).c_str(), &resource.mHighlightType);
    //                yaget::gui::SetTooltip("Highlight resources with same types.");
    //                ImGui::SameLine();

    //                ImGui::Checkbox(fmt::format("H##{}", index).c_str(), &resource.mHighlightHash);
    //                yaget::gui::SetTooltip("Highlight resources with same hash.");
    //                ImGui::SameLine();

    //                ImGui::Checkbox(fmt::format("N##{}", index).c_str(), &resource.mHighlightName);
    //                yaget::gui::SetTooltip("Highlight resources with same names");
    //                ImGui::SameLine();
    //            });

    //            std::string userText = resource.mUserText.empty() ? "" : " (" + resource.mUserText + ")";

    //            float alphaAdjuster = defaultAlphaAdjuster;
    //            if (typeFilter == resource.mResource->GetNameType() || hashFilter == fmt::format("{}", resource.mResource->GetStateHash()) || nameFilter == Section(resource.mResource->mAssetTag).ToString())
    //            {
    //                alphaAdjuster = 1.0f;
    //            }

    //            yaget::gui::TextColors(
    //                keyColor * alphaAdjuster, fmt::format("{:3} - Type:", index),
    //                valueColor * alphaAdjuster, fmt::format(ValueFormater, resource.mResource->GetNameType()),
    //                infoColor * alphaAdjuster, fmt::format(UserFormater, userText),
    //                valueColor * alphaAdjuster, ",",
    //                keyColor * alphaAdjuster, "Name:",
    //                sectionColor * alphaAdjuster, Section(resource.mResource->mAssetTag).ToString());
    //        }
    //        else
    //        {
    //            yaget::gui::TextColors(activeColor, resource.mUserText);
    //        }

    //        ++index;
    //    }

    //    ImGui::End();
    //}
}


void yaget::render::Device::ActivatedResource(const ResourceView* resource, const char* userText /*= nullptr*/)
{
    mFrameResources.push_back({ resource, (userText ? userText : "") });

    if (mResourceBreakOn > -1 && mResourceBreakOn < mFrameResources.size())
    {
        platform::DebuggerBreak();
        mResourceBreakOn = -1;
    }
}
