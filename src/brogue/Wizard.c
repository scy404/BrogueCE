/*
 *  Wizard.c
 *  Brogue
 *
 *  Copyright 2012. All rights reserved.
 *
 *  This file is part of Brogue.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Rogue.h"
#include "GlobalsBase.h"
#include "Globals.h"

static void initializeCreateItemButton(brogueButton *button, char *text) {
    char buttonText[COLS * 3];

    initializeButton(button);
    strcpy(buttonText,text);
    upperCase(buttonText);
    strcpy(button->text,buttonText);
}

#define DIALOG_CREATE_ITEM_MAX_BUTTONS 26

// Display a dialog window for the user to select a single entry from a list
static short dialogSelectEntryFromList(
        brogueButton *buttons,
        short buttonCount,
        char *windowTitle
) {

    short x=0, y=0, width=0, height=0;
    screenDisplayBuffer dbuf;
    short i, selectedButton, len, maxLen;
    char buttonText[COLS];

    maxLen = strlen(windowTitle);

    for (i=0; i < buttonCount; i++) {
        buttons[i].flags &= ~(B_WIDE_CLICK_AREA | B_GRADIENT);
        buttons[i].buttonColor = interfaceBoxColor;
        buttons[i].hotkey[0] = 'a' + i;
        buttons[i].hotkey[1] = 'A' + i;
        buttons[i].x = WINDOW_POSITION_DUNGEON_TOP_LEFT.window_x;
        buttons[i].y = WINDOW_POSITION_DUNGEON_TOP_LEFT.window_y + 1 + i;
        if (KEYBOARD_LABELS) {
            sprintf(buttonText, "%c) %s", (int)buttons[i].hotkey[0], buttons[i].text);
            strcpy(buttons[i].text, buttonText);
            len = strlen(buttonText);
            if (len > maxLen) {
                maxLen = len;
            }
        }
    }

    width = maxLen + 1;
    height = buttonCount + 2;
    x = WINDOW_POSITION_DUNGEON_TOP_LEFT.window_x;
    y = WINDOW_POSITION_DUNGEON_TOP_LEFT.window_y;
    clearDisplayBuffer(&dbuf);

    //Dialog Title
    printString(windowTitle, x , y - 1, &itemMessageColor, &interfaceBoxColor, &dbuf);
    //Dialog background
    rectangularShading(x - 1, y - 1, width + 1, height + 1, &interfaceBoxColor, INTERFACE_OPACITY, &dbuf);
    //Display the title/background and save the prior display state
    const SavedDisplayBuffer rbuf = saveDisplayBuffer();
    overlayDisplayBuffer(&dbuf);
    //Display the buttons and wait for user selection
    selectedButton = buttonInputLoop(buttons, buttonCount, x, y, width, height, NULL);
    //Revert the display state
    restoreDisplayBuffer(&rbuf);

    return selectedButton;
}

// Display a dialog window for the user to chose a vorpal enemy. Remove the runic if none selected.
static short dialogCreateItemChooseVorpalEnemy() {
    char buttonText[COLS];
    short i;
    brogueButton buttons[DIALOG_CREATE_ITEM_MAX_BUTTONS];

    for (i = 0; i < MONSTER_CLASS_COUNT; i++) {
        strcpy(buttonText,monsterClassCatalog[i].name);
        initializeCreateItemButton(&(buttons[i]), buttonText);
    }

    return dialogSelectEntryFromList(buttons, i, "Choose a vorpal enemy:");
}

// Display a dialog window for the user to select a runic for the given armor or weapon.
// Assigns the selected runic and vorpal enemy (if applicable) to the item. No return value.
static void dialogCreateItemChooseRunic(item *theItem){
    char buttonText[COLS];
    short i=0, runicOffset =0, noRunic, selectedRunic, selectedVorpalEnemy;
    brogueButton buttons[DIALOG_CREATE_ITEM_MAX_BUTTONS];

    if (!(theItem->category & (WEAPON | ARMOR))) {
        return;
    }

    // Heavy weapons can only have bad runics. The logic below but differs from that used in dungeon generation, which
    // is based on minimum weapon damage, but has the same net effect. Might be nice to consolidate. Perhaps add a flag
    // to the kind (itemtable).
    if (theItem->category == WEAPON) {
        if (theItem->kind == HAMMER || theItem->kind == WAR_AXE || theItem->kind == PIKE || theItem->kind == BROADSWORD) {
            for (i=0; i<NUMBER_WEAPON_RUNIC_KINDS - NUMBER_GOOD_WEAPON_ENCHANT_KINDS; i++) {
                strcpy(buttonText, weaponRunicNames[i + NUMBER_GOOD_WEAPON_ENCHANT_KINDS]);
                initializeCreateItemButton(&(buttons[i]), buttonText);
            }
            runicOffset = NUMBER_GOOD_WEAPON_ENCHANT_KINDS;
            // Other weapons (except thrown), can have any runic
        } else if (theItem->quiverNumber == 0) {
            for (i=0; i<NUMBER_WEAPON_RUNIC_KINDS; i++) {
                strcpy(buttonText, weaponRunicNames[i]);
                initializeCreateItemButton(&(buttons[i]), buttonText);
            }
        } else { // No runics allowed
            return;
        }
    } else if (theItem->category == ARMOR) {
        if (theItem->kind == PLATE_MAIL) { //bad runics only
            for (i=0; i<NUMBER_ARMOR_ENCHANT_KINDS - NUMBER_GOOD_ARMOR_ENCHANT_KINDS; i++) {
                strcpy(buttonText, armorRunicNames[i + NUMBER_GOOD_ARMOR_ENCHANT_KINDS]);
                initializeCreateItemButton(&(buttons[i]), buttonText);
            }
            runicOffset = NUMBER_GOOD_ARMOR_ENCHANT_KINDS;
        } else {
            for (i=0; i<NUMBER_ARMOR_ENCHANT_KINDS; i++) { //any runic
                strcpy(buttonText, armorRunicNames[i]);
                initializeCreateItemButton(&(buttons[i]), buttonText);
            }
        }
    }
    // add an extra button for choosing no runic
    initializeCreateItemButton(&(buttons[i]), "No Runic");
    noRunic = i;

    selectedRunic = dialogSelectEntryFromList(buttons, i+1, "Choose a runic:");

    if (selectedRunic >=0 && selectedRunic != noRunic) {
        theItem->enchant2 = selectedRunic + runicOffset;
        theItem->flags |= ITEM_RUNIC;

        if ((theItem->enchant2 == W_SLAYING && theItem->category == WEAPON)
            || (theItem->enchant2 == A_IMMUNITY && theItem->category == ARMOR)) {
            selectedVorpalEnemy = dialogCreateItemChooseVorpalEnemy();

            if (selectedVorpalEnemy >=0) {
                theItem->vorpalEnemy = selectedVorpalEnemy;
            } else { // remove the runic if no vorpal enemy chosen
                theItem->enchant2 = 0;
                theItem->flags &= ~(ITEM_RUNIC);
            }
        }
    } else if (selectedRunic == noRunic) {
        theItem->enchant2 = 0;
        theItem->flags &= ~(ITEM_RUNIC);
    }
}

// Display a dialog window for the user to select an item kind from the specified catogory. Returns the selected kind.
static short dialogCreateItemChooseKind(enum itemCategory category) {
    char buttonText[COLS];
    short i;
    brogueButton buttons[DIALOG_CREATE_ITEM_MAX_BUTTONS];
    itemTable *kindTable;
    int kindCount;
    char title[50] = "";

    kindTable = tableForItemCategory(category);
    kindCount = itemKindCount(category, 0);

    for (i=0; i<kindCount; i++) {
        strncpy(buttonText, kindTable[i].name, COLS);
        initializeCreateItemButton(&(buttons[i]), buttonText);
    }

    sprintf(title,"Create %s:",itemCategoryNames[unflag(category)]);
    return dialogSelectEntryFromList(buttons, i, title);
}

// Display an input dialog for the user to enter the enchantment level for the item being created.
// Assigns the enchantment level to the item. Does not return a value.
static void dialogCreateItemChooseEnchantmentLevel(item *theItem, wizardItemOptions *options) {
    short minVal = 0, maxVal = 50, defaultVal, maxInputLength = 2;
    char prompt[100], buf[100], inputBuf[100]="";

    defaultVal = theItem->enchant1;

    // Item generation creates valid enchantment values except for the case of runics. So in all other cases we can
    // retain the generated enchantment value as the default, if the user doesn't provide a value.
    if (theItem->category == WAND) {
        minVal = 0;
        defaultVal = theItem->charges;
    } else if (theItem->category == CHARM) {
        minVal = 1;
    } else if (theItem->category == STAFF) {
        minVal = 2;
    } else if (theItem->category == RING) {
        minVal = -3;
    } else if (theItem->category & (WEAPON | ARMOR)) {
        if (!(theItem->flags & ITEM_RUNIC)) {
            minVal = -3;
        } else { // bad runics can only be negatively enchanted
            if ((theItem->category == ARMOR && theItem->enchant2 >= NUMBER_GOOD_ARMOR_ENCHANT_KINDS)
                || (theItem->category == WEAPON && theItem->enchant2 >= NUMBER_GOOD_WEAPON_ENCHANT_KINDS)) {
                minVal = -3;
                maxVal = -1;
                theItem->enchant1 = rand_range(-3,-1);
            } else { // good runics can only be positively enchanted
                minVal = 1;
                if (theItem->enchant1 < 1) {
                    theItem->enchant1 = rand_range(1,3);
                }
            }
            defaultVal = theItem->enchant1;
        }
    }

    if (!rogue.playbackMode) {
        sprintf(prompt, "How many enchants (%d to %d) [default %d]?", minVal, maxVal, defaultVal);
        getInputTextString(inputBuf, prompt, maxInputLength, "", "", TEXT_INPUT_NORMAL, true);
    }

    // Resolve user input only when live; replay still performs the RNG preparation above.
    int enchants;
    if (rogue.playbackMode && options->enchant != WIZARD_DEFAULT_ENCHANT) {
        sprintf(inputBuf, "%d", options->enchant);
    }
    if (strlen(inputBuf)                      // we need some input
        && sscanf(inputBuf, "%d", &enchants)  // try to convert to number
        && sprintf(buf, "%d", enchants)       // convert back to string
        && strcmp(buf, inputBuf) == 0) {       // compare (0 if equal)
        // handle out of range input
        if (enchants > maxVal) {
            enchants = maxVal;
        } else if (enchants < minVal) {
            enchants = defaultVal;
        }

        options->enchant = enchants;

        // set enchantment level based on item category
        if (theItem->category == WAND) {
            theItem->charges = enchants;
        } else if (theItem->category == STAFF) {
            theItem->charges = theItem->enchant1 = enchants;
        } else {
            theItem->enchant1 = enchants;
        }
    }

    if (theItem->enchant1 < 0) {
        theItem->flags |= ITEM_CURSED;
    }
}

static int creatureTypeCompareMonsterNames (const void * a, const void * b) {
    creatureType *c1 = (creatureType *)a;
    creatureType *c2 = (creatureType *)b;

    char name1[COLS];
    char name2[COLS];
    strncpy(name1,c1->monsterName,COLS);
    strncpy(name2,c2->monsterName,COLS);
    upperCase(name1);
    upperCase(name2);

    return strcmp(name1, name2);
}

// Display a dialog window for the user to select a mutation for the specified monster.
static int dialogCreateMonsterChooseMutation(creature *theMonster) {
    char buttonText[COLS];
    short i, j = 0, noMutation, selectedMutation;
    brogueButton buttons[DIALOG_CREATE_ITEM_MAX_BUTTONS];

    for (i=0; i<NUMBER_MUTATORS; i++) {
        if (!(theMonster->info.flags & mutationCatalog[i].forbiddenFlags)
            && !(theMonster->info.abilityFlags & mutationCatalog[i].forbiddenAbilityFlags)) {
            strncpy(buttonText, mutationCatalog[i].title, COLS);
            initializeCreateItemButton(&(buttons[j]), buttonText);
            buttons[j].command = i; // Store the mutation index (as opposed to the button index)
            j++;
        }
    }
    // add an extra button for choosing no mutation
    initializeCreateItemButton(&(buttons[j]), "No mutation");
    noMutation = j;

    selectedMutation = dialogSelectEntryFromList(buttons, j+1, "Choose a mutation:");

    if (selectedMutation >= 0 && selectedMutation != noMutation) {
        mutateMonster(theMonster, buttons[selectedMutation].command); // Retrieve the mutation index from the button
        return buttons[selectedMutation].command;
    }
    return -1;
}

// Display a series of dialog windows for creating an arbitrary monster chosen by the user
static void dialogCreateMonster() {
    brogueButton buttons[DIALOG_CREATE_ITEM_MAX_BUTTONS];
    char buttonText[COLS];
    short i, j=0, selectedMonster, monsterOffset, buttonCount=0;
    const short MONSTERS_PER_PAGE = 24;
    const short CREATABLE_MONSTER_KINDS = NUMBER_MONSTER_KINDS - 2;
    char monsterRangeText[COLS];
    creatureType monsterKinds[NUMBER_MONSTER_KINDS];

    // There are too many monsters to fit on a page so we break them into ranges. First we copy the monster catalog and
    // sort it alphabetically by name. Also, we exclude creation of lich and phoenix as those are summoned by phylactery
    // and phoenix egg respectively.
    for (i=0; i < NUMBER_MONSTER_KINDS; i++) {
        if (!(monsterCatalog[i].monsterID == MK_LICH) && !(monsterCatalog[i].monsterID == MK_PHOENIX)) {
            monsterKinds[j] = monsterCatalog[i];
            upperCase(monsterKinds[j].monsterName);
            j++;
        }
    }

    qsort(monsterKinds,CREATABLE_MONSTER_KINDS,sizeof(creatureType),&creatureTypeCompareMonsterNames);

    for (i=0; i < CREATABLE_MONSTER_KINDS; i+=MONSTERS_PER_PAGE) {
        if (i + MONSTERS_PER_PAGE < CREATABLE_MONSTER_KINDS) {
            sprintf(monsterRangeText,"%s - %s",monsterKinds[i].monsterName, monsterKinds[i + MONSTERS_PER_PAGE-1].monsterName);
        } else {
            sprintf(monsterRangeText,"%s - %s",monsterKinds[i].monsterName, monsterKinds[CREATABLE_MONSTER_KINDS-1].monsterName);
        }
        initializeCreateItemButton(&(buttons[buttonCount]), monsterRangeText);
        buttonCount++;
    }

    // choose a monster range
    monsterOffset = dialogSelectEntryFromList(buttons, buttonCount, "Create monster:");
    if (monsterOffset == -1) {
        return;
    }
    monsterOffset*= MONSTERS_PER_PAGE;

    // populate menu of monsters in the selected range
    buttonCount = 0;
    for (i=monsterOffset; i < monsterOffset + MONSTERS_PER_PAGE && i < CREATABLE_MONSTER_KINDS; i++) {
        strncpy(buttonText, monsterKinds[i].monsterName, COLS);
        initializeCreateItemButton(&(buttons[buttonCount]), buttonText);
        buttonCount++;
    }

    // choose a monster
    selectedMonster = dialogSelectEntryFromList(buttons, buttonCount, "Create monster:");
    if (selectedMonster == -1) {
        return;
    }

    selectedMonster+= monsterOffset;

    wizardMonsterOptions options = {0};
    options.kind = monsterKinds[selectedMonster].monsterID;
    options.mutation = -1;
    wizardCreateMonster(&options);
}

static boolean wizardMonsterLocationValid(int kind, pos location) {
    if (location.x < 0 || location.x >= DCOLS || location.y < 0 || location.y >= DROWS
        || !playerCanSeeOrSense(location.x, location.y) || posEq(location, player.loc)) {
        return false;
    }
    if (monsterCatalog[kind].displayChar == G_TURRET) {
        return pmapAt(location)->layers[DUNGEON] == WALL;
    }
    return !cellHasTerrainFlag(location, T_OBSTRUCTS_PASSABILITY);
}

boolean wizardMonsterOptionsValid(const wizardMonsterOptions *o) {
    if (o->kind < 0 || o->kind >= NUMBER_MONSTER_KINDS
        || o->kind == MK_LICH || o->kind == MK_PHOENIX
        || o->mutation < -1 || o->mutation >= NUMBER_MUTATORS
        || o->outcome < WIZARD_MONSTER_CANCELED || o->outcome > WIZARD_MONSTER_PLACED) {
        return false;
    }
    const creatureType *type = &monsterCatalog[o->kind];
    if (o->mutation >= 0 && ((type->flags & MONST_NEVER_MUTATED)
        || (type->abilityFlags & MA_NEVER_MUTATED)
        || (type->flags & mutationCatalog[o->mutation].forbiddenFlags)
        || (type->abilityFlags & mutationCatalog[o->mutation].forbiddenAbilityFlags))) {
        return false;
    }
    if (o->outcome == WIZARD_MONSTER_CANCELED) return !o->ally;
    if (o->location.x < 0 || o->location.x >= DCOLS
        || o->location.y < 0 || o->location.y >= DROWS) {
        return false;
    }
    boolean valid = wizardMonsterLocationValid(o->kind, o->location);
    if (o->outcome == WIZARD_MONSTER_INVALID_LOCATION) return !valid && !o->ally;
    if (!valid) {
        return false;
    }
    if (o->ally && (type->flags & (MONST_INANIMATE | MONST_INVULNERABLE))
        && o->kind != MK_PHOENIX_EGG && o->kind != MK_PHYLACTERY) {
        return false;
    }
    return true;
}

void wizardCreateMonster(wizardMonsterOptions *options) {
    char text[COLS];
    rogueEvent event = {0};
    if (!WIZARD_MODE || !wizardMonsterOptionsValid(options)) return;
    creature *theMonster = generateMonster(options->kind, false, false);
    if (!rogue.playbackMode) {
        if (!(theMonster->info.flags & MONST_NEVER_MUTATED)
            && !(theMonster->info.abilityFlags & MA_NEVER_MUTATED)) {
            options->mutation = dialogCreateMonsterChooseMutation(theMonster);
        }
    } else if (options->mutation >= 0) mutateMonster(theMonster, options->mutation);
    initializeMonster(theMonster, false);
    if (!rogue.playbackMode) {
        sprintf(
            text, "Create %s where? Choose a visible %s.", theMonster->info.monsterName,
            theMonster->info.displayChar == G_TURRET ? "wall" : "unobstructed location"
        );
        temporaryMessage(text, REFRESH_SIDEBAR);
        if (!chooseTarget(&options->location, 0, AUTOTARGET_MODE_NONE, NULL)) {
            options->location = (pos){0, 0};
            options->outcome = WIZARD_MONSTER_CANCELED;
        } else {
            options->outcome = wizardMonsterLocationValid(options->kind, options->location)
                ? WIZARD_MONSTER_PLACED : WIZARD_MONSTER_INVALID_LOCATION;
        }
    }
    confirmMessages();
    if (options->outcome != WIZARD_MONSTER_PLACED) {
        if (options->outcome == WIZARD_MONSTER_INVALID_LOCATION && !rogue.playbackMode) {
            sprintf(
                text, "Invalid location. %s",
                KEYBOARD_LABELS ? "-- Press space or click to continue --"
                    : "-- Touch anywhere to continue --"
            );
            temporaryMessage(text, REQUIRE_ACKNOWLEDGMENT);
        }
        event.eventType = WIZARD_CREATE_MONSTER;
        event.wizardMonster = *options;
        recordEvent(&event);
        killCreature(theMonster, true);
        removeDeadMonsters();
        if (options->outcome == WIZARD_MONSTER_CANCELED) refreshSideBar(-1, -1, false);
        return;
    }
    creature *oldMonster = monsterAtLoc(options->location);
    if (oldMonster) { killCreature(oldMonster, true); removeDeadMonsters(); }
    theMonster->loc = options->location;
    pmapAt(theMonster->loc)->flags |= HAS_MONSTER;
    theMonster->creatureState = MONSTER_WANDERING;
    fadeInMonster(theMonster);
    refreshSideBar(-1, -1, false);
    refreshDungeonCell(theMonster->loc);
    if (!(theMonster->info.flags & (MONST_INANIMATE | MONST_INVULNERABLE))
        || theMonster->info.monsterID == MK_PHOENIX_EGG
        || theMonster->info.monsterID == MK_PHYLACTERY) {
        if (!rogue.playbackMode) {
            sprintf(text, "Make the %s your ally?", theMonster->info.monsterName);
            options->ally = confirm(text, false);
        }
        if (options->ally) becomeAllyWith(theMonster);
    }
    event.eventType = WIZARD_CREATE_MONSTER;
    event.wizardMonster = *options;
    recordEvent(&event);
    sprintf(text, "%s%s created.", options->ally ? "Allied " : "", theMonster->info.monsterName);
    message(text, 0);
    refreshSideBar(-1, -1, false);
}


// Shared by the interactive wizard command and recording playback. Generate the
// quantity again rather than recording it, preserving substantive RNG consumption.
void wizardCreateGold(void) {
    item *theItem;
    char text[COLS];
    rogueEvent event = {0};

    if (!WIZARD_MODE || numberOfItemsInPack() == MAX_PACK_ITEMS) {
        return;
    }
    event.eventType = WIZARD_CREATE_GOLD;
    recordEvent(&event);
    theItem = generateItem(GOLD, 0);
    rogue.gold += theItem->quantity;
    sprintf(
        text, "you found %i pieces of gold.%s", theItem->quantity,
        rogue.featRecord[FEAT_TONE] ? ".. and strike a tone of disappointment." : ""
    );
    rogue.featRecord[FEAT_TONE] = false;
    messageWithColor(text, &itemMessageColor, 0);
    deleteItem(theItem);
}

// Display a series of dialog windows for creating an arbitrary item chosen by the user
static void dialogCreateItem() {
    brogueButton buttons[DIALOG_CREATE_ITEM_MAX_BUTTONS];
    char buttonText[COLS];
    short i, selectedCategory, selectedKind;

    if (numberOfItemsInPack() == MAX_PACK_ITEMS) {
        messageWithColor("Your pack is already full.", &itemMessageColor, 0);
        return;
    }

    for (i=0; i < NUMBER_ITEM_CATEGORIES; i++) {
        strncpy(buttonText, itemCategoryNames[i], COLS);
        initializeCreateItemButton(&(buttons[i]), buttonText);
    }

    selectedCategory = dialogSelectEntryFromList(buttons, i, "Create item:");

    if (selectedCategory == -1) {
        return;
    } else if (tableForItemCategory(Fl(selectedCategory))) {
        selectedKind = dialogCreateItemChooseKind(Fl(selectedCategory));
    } else {
        selectedKind = 0;
    }

    if (Fl(selectedCategory) == GOLD) {
        wizardCreateGold();
        return;
    }

    if (selectedKind < 0) return;
    wizardItemOptions options = {0};
    options.category = selectedCategory;
    options.kind = selectedKind;
    options.enchant = WIZARD_DEFAULT_ENCHANT;
    wizardCreateItem(&options);
}

boolean wizardItemOptionsValid(const wizardItemOptions *o) {
    if (o->category < 0 || o->category >= NUMBER_ITEM_CATEGORIES || Fl(o->category) == GOLD
        || o->kind < 0 || o->runic < 0
        || o->vorpalEnemy < 0 || o->vorpalEnemy >= MONSTER_CLASS_COUNT
        || (o->enchant != WIZARD_DEFAULT_ENCHANT && (o->enchant < -3 || o->enchant > 50))) {
        return false;
    }
    enum itemCategory category = Fl(o->category);
    int count = tableForItemCategory(category) ? itemKindCount(category, 0) : 1;
    if (o->kind >= count) {
        return false;
    }
    if (category == WEAPON) {
        if (o->runic >= NUMBER_WEAPON_RUNIC_KINDS) {
            return false;
        }
        if (o->kind >= DART && o->hasRunic) {
            return false;
        }
        if ((o->kind == HAMMER || o->kind == WAR_AXE || o->kind == PIKE || o->kind == BROADSWORD)
            && o->hasRunic && o->runic < NUMBER_GOOD_WEAPON_ENCHANT_KINDS) {
            return false;
        }
    } else if (category == ARMOR) {
        if (o->runic >= NUMBER_ARMOR_ENCHANT_KINDS) {
            return false;
        }
        if (o->kind == PLATE_MAIL && o->hasRunic && o->runic < NUMBER_GOOD_ARMOR_ENCHANT_KINDS) {
            return false;
        }
    } else if (o->runic || o->hasRunic) {
        return false;
    }
    if (!(category & CAN_BE_ENCHANTED) && o->enchant != WIZARD_DEFAULT_ENCHANT) {
        return false;
    }
    if ((category & NEVER_IDENTIFIABLE) && o->identify) {
        return false;
    }
    return true;
}

void wizardCreateItem(wizardItemOptions *options) {
    char text[COLS], name[100];
    rogueEvent event = {0};
    if (!WIZARD_MODE || numberOfItemsInPack() == MAX_PACK_ITEMS
        || !wizardItemOptionsValid(options)) {
        return;
    }
    enum itemCategory category = Fl(options->category);
    item *theItem = generateItem(category, options->kind);
    theItem->flags &= ~ITEM_CURSED;
    if (category == GEM) theItem->originDepth = rogue.depthLevel;

    if (category & (ARMOR | WEAPON)) {
        if (!rogue.playbackMode) {
            dialogCreateItemChooseRunic(theItem);
            options->runic = theItem->enchant2;
            options->hasRunic = (theItem->flags & ITEM_RUNIC) != 0;
            options->vorpalEnemy = theItem->vorpalEnemy;
        } else {
            theItem->enchant2 = options->runic;
            if (options->hasRunic) theItem->flags |= ITEM_RUNIC;
            else theItem->flags &= ~ITEM_RUNIC;
            theItem->vorpalEnemy = options->vorpalEnemy;
        }
    }
    if ((category & CAN_BE_ENCHANTED) && theItem->quiverNumber == 0) {
        dialogCreateItemChooseEnchantmentLevel(theItem, options);
    }
    if ((theItem->flags & ITEM_CAN_BE_IDENTIFIED) && !(category & NEVER_IDENTIFIABLE)) {
        if (!rogue.playbackMode) options->identify = confirm("Identify the item?", false);
        if (options->identify) identify(theItem);
    }
    event.eventType = WIZARD_CREATE_ITEM;
    event.wizardItem = *options;
    recordEvent(&event);
    theItem = addItemToPack(theItem);
    itemName(theItem, name, true, true, NULL);
    sprintf(text, "you now have %s (%c).", name, theItem->inventoryLetter);
    messageWithColor(text, &itemMessageColor, 0);
}

void dialogCreateItemOrMonster() {
    brogueButton buttons[DIALOG_CREATE_ITEM_MAX_BUTTONS];
    short selectedType;

    initializeCreateItemButton(&(buttons[0]), "Item");
    initializeCreateItemButton(&(buttons[1]), "Monster");

    selectedType = dialogSelectEntryFromList(buttons, 2, "Create:");

    if (selectedType == 0) {
        dialogCreateItem();
    } else if (selectedType == 1) {
        dialogCreateMonster();
    }
}
