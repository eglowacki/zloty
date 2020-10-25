///////////////////////////////////////////////////////////////////////
// GameTypes.h
//
//  Copyright 10/10/2020 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides helper and synthetic sugar for types used in game
//
//
//  #include "GameTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/GameCoordinator.h"

namespace ttt
{
    class BoardComponent;
    class ScoreComponent;
    class InputComponent;
    class PlayerComponent;
    class InventoryComponent;
    class PieceComponent;

    // Represents type of pieces we have between players
    // It also serves as side control for player
    enum class PieceType {Blank, X, O };

    //! We only have one board and score hud per match
    //using set_coordinators = bool;
    struct GlobalEntity : yaget::comp::RowPolicy<BoardComponent*, ScoreComponent*>
    {
        using AutoCleanup = bool;
        using Global = bool;
    };

    //! This represents allowable components that can form entity
    //! For this game, we will have player consist of:
    //! Input, Player, Inventory
    //!
    //! and entities representing pieces on a board, which are own by player inventory until placed on a board by player
    //! Piece

    struct Entity : yaget::comp::RowPolicy<InputComponent*, PlayerComponent*, InventoryComponent*, PieceComponent*>
    {
        using AutoCleanup = bool;
    };

}
