#pragma once

// Roster IDs used by player select and battle fighter factory.
enum CharacterId {
    Char_Makoto = 0,
    Char_Joker,
    Char_Narukami,
    Char_Count
};

inline const char* GetCharacterDisplayName(CharacterId id) {
    switch (id) {
    case Char_Makoto: return "MAKOTO";
    case Char_Joker: return "JOKER";
    case Char_Narukami: return "NARUKAMI";
    default: return "???";
    }
}

inline const char* GetCharacterIconPath(CharacterId id) {
    switch (id) {
    case Char_Makoto: return "assets/makoto/makoto_icon.png";
    case Char_Joker: return "assets/joker/joker_icon.png";
    case Char_Narukami: return "assets/narukami/narukami_icon.png";
    default: return "assets/makoto/makoto_icon.png";
    }
}
