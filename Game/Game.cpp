#include "../shared/pch.h"
#include "../shared/shared.h"

#include "Game.h"

std::string step_to_str(GameStep step)
{
    switch (step)
    {
        case GameStep_Begin :
        {
            return "Begin";
        } break;
        case GameStep_Question :
        {
            return "Question";
        } break;
        case GameStep_Result :
        {
            return "Result";
        } break;
        case GameStep_EndResults :
        {
            return "End Results";
        } break;
        case GameStep_None :
        default :
        {
            jassertfalse;
            return "";
        };
    }
}

//TODO helloooooooo tu vas mourir toi
Widget_Interaction_Type gameStepToFaderStep(GameStep game_step, Mix mix)
{
    switch (game_step)
    {
        case GameStep_Begin :
        {
            assert(mix == Mix_Hidden);
            return Widget_Editing;
        } break;
        case GameStep_Question :
        {
            if (mix == Mix_User)
            {
                return Widget_Editing;
            }
            else if (mix == Mix_Target)
            {
                return Widget_Hiding;
            }
            else
            {
                jassertfalse;
                return {};
            }
        } break;
        case GameStep_Result :
        {
            assert(mix != Mix_Hidden);
            return Widget_Showing;
        } break;
        case GameStep_EndResults :
        case GameStep_None :
        default :
        {
            jassertfalse;
            return {};
        };
    }
}
