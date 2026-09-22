#include <a_samp>

native PB_RegisterBot(const nickname[]);

new const BotNames[5][MAX_PLAYER_NAME] = {
    "Bot_David", "Bot_Ivan", "Bot_Chris", "Bot_Hasan", "Bot_Mustapha"
};

main() {}

public OnGameModeInit()
{
    SetGameModeText("sampbot ci");
    for (new i = 0; i < sizeof(BotNames); i++)
    {
        PB_RegisterBot(BotNames[i]);
        ConnectNPC(BotNames[i], "npcidle");
    }
    return 1;
}
