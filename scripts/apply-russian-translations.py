#!/usr/bin/env python3
"""Apply the curated Russian catalog derived from the localized Rune War resources."""

from __future__ import annotations

import ast
import re
import sys
from pathlib import Path


TRANSLATIONS: dict[str, str] = {
    "Original Theme": "Оригинальная тема",
    "Select Wizard & Clan": "Колдун и Клан",
    "Wizard:": "Колдун:",
    "Clan:": "Клан:",
    "Epithet:": "Титул:",
    "Clans:": "Кланы:",
    "Creature:": "Существа:",
    "Empty Stone": "Пустая руна",
    "East Wind": "Восточный ветер",
    "South Wind": "Южный ветер",
    "West Wind": "Западный ветер",
    "Nord Wind": "Северный ветер",
    "White Dragon": "Белый Дракон",
    "Green Dragon": "Зелёный Дракон",
    "Red Dragon": "Красный Дракон",
    "East": "Восток",
    "South": "Юг",
    "West": "Запад",
    "North": "Север",
    "Red": "Красный",
    "Yellow": "Жёлтый",
    "Aqua": "Бирюзовый",
    "Purple": "Фиолетовый",
    "Orachi": "Орачин",
    "Lakkho": "Лакхор",
    "Dayla": "Дайла",
    "Ziag": "Зиаг",
    "Niana": "Ниана",
    "Kierac": "Киерак",
    "Logun": "Логун",
    "Nucrus": "Нукрус",
    "Javed": "Жавед",
    "Skeleton Horde": "Орда Скелетов",
    "Shadow": "Гнильё",
    "Durlock": "Драйдер",
    "Wraith": "Чучело",
    "Maz'Ra": "Маз'Ра",
    "Knight Templar": "Отряд Рыцарей",
    "Minotaur": "Минотавр",
    "Adventure Party": "Приключенцы",
    "Fire Giant": "Пламенник",
    "Sand Wraith": "Дух Пустыни",
    "Stone Golem": "Каменный Голем",
    "Thunder Bird": "Птица Молний",
    "Ki-Lin": "Ки-Рин",
    "Tornado": "Торнадо",
    "Griffon": "Грифон",
    "Chameleon": "Хамелеон",
    "Kilor Celsbane": "Килор Вурмбэйн",
    "Carol the Great": "Великий Профосс",
    "King Drago": "Король Уго",
    "Learra the Siren": "Сирена Ниеннах",
    "Shanahan": "Шаннаган",
    "Fire Elemental": "Элементаль Огня",
    "Earth Elemental": "Элементаль Земли",
    "Air Elemental": "Элементаль Эфира",
    "Water Elemental": "Элементаль Воды",
    "Juggernaut": "Джагернаут",
    "Smoke": "Туман",
    "Demonic Compulsion": "Влияние Демонишек",
    "Mass Panic": "Общий Ужас",
    "Paralyze": "Паралич",
    "Reduction": "Редукция",
    "Battle Fury": "Ярость Боя",
    "Guidance": "Наведение",
    "Force Shield": "Силовой Щит",
    "Dust Cloud": "Облако Пыли",
    "Heroism": "Героизм",
    "Mystical Fountain": "Мистический Ручей",
    "Blind Ambition": "Слепая Амбиция",
    "Brilliant Lights": "Яркое Свечение",
    "Magical Aura": "Аура Волшбы",
    "Teleport": "Телепорт",
    "Dispel Magic": "Разрушение Магии",
    "Lightning Bolt": "Шаровая Молния",
    "Healing": "Лечение",
    "Hell Blast": "Взрыв Огня",
    "Summon Skull Rune": "Вызвать Руну Черепа",
    "Summon Sword Rune": "Вызвать Руну Меча",
    "Summon Number Rune": "Вызвать Руну Чисел",
    "Random Discard": "Случайная Ставка",
    "Scry Runes": "Ясновидение",
    "Silence": "Тишина",
    "Mana Fog": "Дым Маны",
    "Mass Dispel Magic": "Общее Разрушение Магии",
    "Lord of the Dead": "Владыка Смертей",
    "High Lord Templar": "Король-Тамплиер",
    "Sea Princess": "Королева Вод",
    "The Tactician": "Богдо-Гэгэн",
    "Enchantress of Fire": "Огненная Волшебница",
    "The Shapeshifter": "Изменяющий Плоть",
    "Minstrel of Magic": "Менестрель Магии",
    "The Trickster": "Чёрный Трюкач",
    "Master of the Skies": "Небесный Повелитель",
    "%1's monocle allows him to see all invisible units on the map.":
        "Монокль %1 позволяет ему видеть все невидимые отряды на карте.",
    "%1's catastrophic Hell Blast can wipe out entire army lines and is unique to her.":
        "Катастрофический Взрыв Огня %1 способен уничтожать целые ряды армий; этой силой владеет только она.",
    "Being a bard, %1 is loved by all people and creatures around him. His songs inspire bravery which cause his creatures to regain 2 loyalty each round.":
        "Как бард, %1 любим всеми людьми и существами вокруг. Его песни вселяют отвагу, поэтому его существа восстанавливают 2 Верности каждый раунд.",
    "%1 possesses uncanny luck in the Rune Game. On each unforced turn draw, he sees two runes and keeps one; the other returns to the bottom of the wall. Directed draw spells take precedence.":
        "%1 обладает невероятной удачей в Игре Рун. При каждом обычном доборе в свой ход он видит две руны и оставляет одну; другая возвращается под стену. Заклинания направленного добора имеют приоритет.",
    "%1 is a telepath. This allows him to chose, pung, kong, game and cast spells even when silence has been cast.":
        "%1 — телепат. Это позволяет ему объявлять Чоу, Панг, Конг и Маджонг, а также применять заклинания даже под действием Тишины.",
    "Tower of 4 Winds": "Башня 4 Ветров",
    "Maithaius": "Майтхаеус",
    "Baliphon": "Балифонг",
    "Vermille": "Вермилле",
    "Sulanthia": "Сулантирг",
    "Trojensek": "Троженсец",
    "Talon": "Налот",
    "Siramak": "Камарис",
    "Ronzinol": "Ронзинол",
    "Corzen": "Горзен",
    "Greenbaw": "Редлохиб",
    "Zubrus": "Сурраг",
    "Corimar": "Рутьлек",
    "Inkartha": "Алькарот",
    "Hexan": "Стурц",
    "Firland": "Тарланд",
    "Vesna": "Сприн",
    "Kern": "Карн",
    "Regency Peaks": "Пик Регентств",
    "Knighthaven": "Найтхейвен",
    "Rikter": "Риктер",
    "Gorok": "Корог",
    "Suntura": "Сунтуро",
    "Bertram": "Бертрам",
    "Mastenbroek": "Мастенброек",
    "Reisse": "Реиссе",
    "Cirrus": "Квакус",
    "Grosek": "Хэбэйк",
    "Chahrr": "Чахарр",
    "Saugrezz": "Саугрезз",
    "Mahnjeet": "Махагонс",
    "Trassk": "Денвер",
    "Bechnarr": "Бехнарро",
    "Azuria": "Азурак",
    "Pyramus Reach": "Йотунсхейм",
    "Serenity Plains": "Степи Молчания",
    "Sunspot": "Суданг",
    "Charmers Expanse": "Гьялархейм",
    "Gambit's Run": "Перекатиполе",
    "Ash Point": "Кейп-Нон",
    "Orchid Bay": "Бейкочинос",
    "Mocklebury": "Моклесбери",
    "Celestial Wood": "Заросли Небес",
    "Tortoise Isle": "Тортуга-Ислет",
    "Siphon's Chute": "Гиппогриффланд",
    "Unique": "Уникум",
    "Speciality": "Особенность",
    "Ability": "Способность",
    "Curse": "Проклятие",
    " (unique)": " (уникум)",
    " (curse)": " (проклятие)",
    "Attack:": "Атака:",
    "Ranger:": "Стрельба:",
    "Defense:": "Защита:",
    "Loyalty:": "Верность:",
    "Move Point:": "Движение:",
    "Effect: ": "Эффект: ",
    "Target: ": "Цель: ",
    "Land Claims": "Запросы на Землю",
    "Spell Points": "Очки Магии",
    "Spell Points: %{points}": "Очки Магии: %{points}",
    "Win Rune": "Победная Руна",
    "Sets:": "Наборы:",
    "Points:": "Очки:",
    "Doubles:": "Множители:",
    "Base Score:": "Базовые Очки:",
    "Total Points:": "Всего Очков:",
    "Multiplier:": "Множитель:",
    "Total Score:": "Общий Счёт:",
    "Total Score": "Общий Счёт",
    "Score": "Счёт",
    "Rank": "Ранг",
    "Category": "Категория",
    "Territory Score": "Очки Территорий",
    "Summon Circle Score": "Очки Кругов Призыва",
    "Unit Score": "Очки Войск",
    "Spell Point Score": "Очки Магии",
    "Land Claim Score": "Поземельные Очки",
    "Combat": "Битва",
    "Summary": "Итоги",
    "Winner is": "Побеждает",
    "Combat Summary": "Итоги Битвы",
    "Battle Summary": "Итоги Битвы",
    "Game Summary": "Итоги Игры",
    "Mahjong Summary": "Итоги Маджонга",
    "Adventure": "Война Рун",
    "Mahjong": "Маджонг",
    "Movement Phase": "Фаза Движения",
    "Order": "Приказ",
    "Order Of Play": "Порядок Игры",
    "Select destination": "Выберите Землю",
    "Selected creatures cannot move to this territory.":
        "Выбранные существа не могут перейти на эту землю.",
    "View Map": "Смотреть Карту",
    "Summoning": "Призвание",
    "Info": "Инфо",
    "Random": "Случай",
    "Supplemental Spells:": "Добавочные Заклятья:",
    "Special Abilities:": "Специальные Силы:",
    "You Are %1": "Вы — %1",
    "%{name}'s Spells": "Заклинания: %{name}",
    "AI Level": "Сложность",
    "Difficulty": "Сложность",
    "Easy": "Легко",
    "Normal": "Норма",
    "Hard": "Сложно",
    "Battle Forecast": "Прогноз Боя",
    "Capture: %1%": "Захват: %1%",
    "Attackers remain: %1%": "Выживет Атакующих: %1%",
    "Luck: choose a rune": "Удача: Выберите Руну",
    "The other rune returns to the wall.": "Другая руна вернётся в стену.",
    "One Chance": "Один Шанс",
    "Self Drawn": "Сам Выбрал",
    "All Concealed, with Discard": "Всё Скрыто, Считая Ставку",
    "Lucky Sets": "Набор Удач",
    "One Suit plus Honors": "Одна Масть и Почести",
    "No Points": "Нет Очков",
    "Four Triplets": "Четыре Тройки",
    "Terminals and Honors": "Терминалы и Почести",
    "Terminal, Honor in each set": "Терминал или Почесть в Каждом Наборе",
    "All Simples": "Все Простые",
    "Three Little Dragons": "Три Малых Дракона",
    "One Suit Only": "Одна Масть",
    "Three Consecutive Sequences": "Три Последовательности",
    "All Concealed, Self Drawn": "Всё Скрыто, Сам Выбрал",
    "Last Tile": "Последняя Руна",
    "Three Concealed Triplets": "Три Скрытые Тройки",
    "Robbed a Kong": "Забрал Конг",
    "Supplemental Tile": "Дополнительная Руна",
    "Three Big Dragons": "Три Больших Дракона",
    "Big Four Winds": "Четыре Больших Ветра",
    "All Terminals": "Все Терминалы",
    "All Honors": "Все Почести",
    "Nine Gates": "Девять Врат",
    "Heavenly Hand": "Небесная Рука",
    "Four Concealed Triplets": "Четыре Скрытые Тройки",
    "Thirteen Orphans": "Тринадцать Сирот",
    "Little Four Winds": "Четыре Малых Ветра",
    "Earthly Hand": "Рука Земли",
    "Game Drawn": "Вничью...",
    "Drawn": "Сам Выбрал",
    "%1 Round: %2 Deal": "Раунд %1: Сдаёт %2",
    "%1 (%2) wins from %3": "%1 (%2) побеждает благодаря %3",
    "%1 wins %2 x%3 from %4 = %5": "%1 получает у %4: %2 × %3 = %5",
    "%1 in %2": "%1 в %2",
    "%1's %2 was vanquished": "%2 игрока %1 повержен",
    "Apply spell: %1, on land: %2?": "Применить заклинание %1 на земле %2?",
    "Can'not apply spell: %1, incorrect target: %2":
        "Нельзя применить заклинание %1: неверная цель %2",
    "Teleport selected creature to %1?": "Телепортировать выбранное существо в %1?",
    "Select another friendly territory with a free party slot.":
        "Выберите другую дружественную землю со свободным местом в отряде.",
    "Now it is not your turn. Sorry...": "Сейчас не ваш ход...",
    "Game Begins With %1": "Игра начинается с ветра %1",
    "%1 Call Game": "%1 объявляет Маджонг",
    "%1 Kongs The Last Discard": "%1 собирает Конг на сброшенной руне",
    "%1 Kongs The Last Drawn Rule": "%1 собирает Конг на взятой руне",
    "%1 Pungs The Last Discard": "%1 собирает Панг на сброшенной руне",
    "%1 Chows The Last Discard": "%1 собирает Чоу на сброшенной руне",
    "%1 summons %2 in %3": "%1 призывает %2 в %3",
    "%1 casts %2 over %3": "%1 применяет %2 на земле %3",
    "%1 casts %2": "%1 применяет %2",
    "%1 casts %2 to %3": "%1 применяет %2 к %3",
    "%1 casts %2 at %3's %4": "%1 применяет %2 к существу %4 игрока %3",
    "%1 casts %2 at friendly %3": "%1 применяет %2 к союзному существу %3",
    "Magic Resistance!": "Сопротивление магии!",
    "Exit game?": "Выйти из игры?",
    "Creature: %1, stats [%2, %3, %4, %5]": "Существо: %1, параметры [%2, %3, %4, %5]",
    "Spell: %1, target: %2, %3": "Заклинание: %1, цель: %2, %3",
    "Clan: %1": "Клан: %1",
    "Avatar: %1": "Колдун: %1",
    "Freeze creature": "Обездвижить существо",
    "Move one unit to any friendly territory": "Переместить отряд в дружественную землю",
    "Remove enchantments": "Снять все эффекты магии",
    "Missile attacks do %1 less damage": "Выстрелы наносят на %1 меньше урона",
    "Shannahan heals 2 loyalty before movement phase.":
        "Шаннаган восстанавливает 2 верности перед фазой движения.",
    "Fire Shield": "Огненный Щит",
    "Mighty Blow: ": "Мощный Удар: ",
    "Melee": "Рукопашная",
    "Melee round": "Рукопашный Раунд",
    "Missile volley": "Залп",
    "Missile assignment": "Выбор Стрелков",
    "Opening leader": "Первый Боец",
    "Auto Resolve": "Автобой",
    "Choose the opening leader. Enter: recommended. Esc: Auto Resolve.":
        "Выберите первого бойца. Enter: рекомендация. Esc: автобой.",
    "Choose a missile attacker. Enter: recommended. Esc: Auto Resolve.":
        "Выберите стрелка. Enter: рекомендация. Esc: автобой.",
    "Choose a missile target. Enter: recommended. Esc: Auto Resolve.":
        "Выберите цель стрелка. Enter: рекомендация. Esc: автобой.",
    "Choose an attacker. Enter: recommended. Esc: Auto Resolve.":
        "Выберите атакующего. Enter: рекомендация. Esc: автобой.",
    "Choose a target. Enter: recommended. Esc: Auto Resolve.":
        "Выберите цель. Enter: рекомендация. Esc: автобой.",
    "Complete": "Завершено",
    "damage": "урон",
    "hit": "удар",
    "Phase: ": "Фаза: ",
    "Events: ": "События: ",
    "Event ": "Событие ",
    "Last event: ": "Последнее событие: ",
    "Preview: ": "Прогноз: ",
    "Speed: ": "Скорость: ",
    "Between rounds": "Между Раундами",
    "Pause": "Пауза",
    "Resume": "Продолжить",
    "Paused - ": "Пауза — ",
    "Main Menu": "Главное Меню",
    "MAIN MENU": "ГЛАВНОЕ МЕНЮ",
    "GAME MENU": "МЕНЮ ИГРЫ",
    "New Game": "Новая Игра",
    "Continue": "Продолжить",
    "Load Game": "Загрузить Игру",
    "LOAD GAME": "ЗАГРУЗКА ИГРЫ",
    "Save Game": "Сохранить Игру",
    "SAVE GAME": "СОХРАНЕНИЕ ИГРЫ",
    "Save": "Сохранить",
    "Load": "Загрузить",
    "Delete": "Удалить",
    "Back": "Назад",
    "Cancel": "Отмена",
    "Apply": "Применить",
    "Quit": "Выйти",
    "Return to Main Menu": "Вернуться в Главное Меню",
    "Encyclopedia": "Энциклопедия",
    "Multiplayer": "Сетевая Игра",
    "Settings": "Настройки",
    "SETTINGS": "НАСТРОЙКИ",
    "Language": "Язык",
    "Game Speed": "Скорость Игры",
    "Classic": "Классика",
    "Fast": "Быстро",
    "English": "Английский",
    "Russian": "Русский",
    "Music": "Музыка",
    "Sound Effects": "Игровые Звуки",
    "Voices": "Озвучка",
    "Guardian Voices": "Голоса Стражей",
    "Display Mode": "Режим Экрана",
    "Windowed": "В Окне",
    "Fullscreen": "Полный Экран",
    "ON": "ВКЛ",
    "OFF": "ВЫКЛ",
    "Changes apply immediately": "Изменения применяются сразу",
    "LEFT / RIGHT TO ADJUST": "← / → — ГРОМКОСТЬ",
    "Could not save settings: %1": "Не удалось сохранить настройки: %1",
    "Could not switch display mode.": "Не удалось переключить режим экрана.",
    "PLANNED": "В ПЛАНАХ",
    "DEV BUILD": "СБОРКА",
    "RELEASE": "РЕЛИЗ",
    "THE RUNE WAR": "ВОЙНА РУН",
    "OLD RUNES,": "СТАРЫЕ РУНЫ,",
    "A NEW WAR": "НОВАЯ ВОЙНА",
    "COMMUNITY RESTORATION": "ВОЗРОЖДЕНО СООБЩЕСТВОМ",
    "OF THE ORIGINAL GAME": "ПО МОТИВАМ ОРИГИНАЛА",
    "ESC / ENTER / CLICK TO SKIP": "ESC / ENTER / КЛИК — ПРОПУСТИТЬ",
    "FOUR": "FOUR",
    "WINDS": "WINDS",
    "REBORN": "REBORN",
    "NO SAVED GAME": "НЕТ АВТОСОХРАНЕНИЯ",
    "INVALID SAVE": "СОХРАНЕНИЕ ПОВРЕЖДЕНО",
    "NO SAVES": "НЕТ СОХРАНЕНИЙ",
    "INVALID": "ПОВРЕЖДЕНО",
    "Loading...": "Загрузка...",
    "Game data rendering...": "Подготовка игры...",
    "Error": "Ошибка",
    "load theme error, see log for details": "Не удалось загрузить тему. Подробности в журнале.",
    "Unable to apply pending orders.": "Не удалось применить подготовленные приказы.",
    "Unable to save game.": "Не удалось сохранить игру.",
    "Unable to update the autosave.": "Не удалось обновить автосохранение.",
    "AUTOSAVE AND NAMED SAVES": "АВТОСОХРАНЕНИЕ И СОХРАНЁННЫЕ ИГРЫ",
    "EMERGENCY RECOVERY": "АВАРИЙНОЕ ВОССТАНОВЛЕНИЕ",
    "Autosave": "Автосохранение",
    "Named save": "Сохранённая Игра",
    "Saved game": "Сохранённая Игра",
    "Saves": "Сохранения",
    "Recovery": "Восстановление",
    "Recovery Warning": "Внимание: Восстановление",
    "Restore Recovery": "Восстановить",
    "Emergency checkpoint %1": "Аварийная Точка %1",
    "Updated automatically": "Обновляется автоматически",
    "Create or overwrite a named save": "Создать или перезаписать сохранение",
    "The autosave will be updated": "Автосохранение будет обновлено",
    "The checkpoint will be left untouched": "Аварийная точка останется нетронутой",
    "The file will be left untouched": "Файл останется нетронутым",
    "The file can be deleted but not loaded": "Файл можно удалить, но нельзя загрузить",
    "Unknown time": "Время неизвестно",
    "Unknown version": "Версия неизвестна",
    "Double-click a valid save to load. Delete removes named saves only.":
        "Дважды щёлкните по исправному сохранению. Удалять можно только именные сохранения.",
    "Double-click or use Load. Restoring recovery replaces the autosave.":
        "Дважды щёлкните или нажмите «Загрузить». Восстановление заменит автосохранение.",
    "No autosave or named save was found.": "Автосохранение и именные сохранения не найдены.",
    "No recovery checkpoint was found.": "Аварийные точки не найдены.",
    "Save name": "Имя Сохранения",
    "Enter a name for this save": "Введите имя сохранения",
    "Enter a save name.": "Введите имя сохранения.",
    "Saved as '%1'.": "Сохранено как «%1».",
    "The game could not be saved: %1": "Не удалось сохранить игру: %1",
    "Overwrite Save": "Перезаписать Сохранение",
    "Delete Save": "Удалить Сохранение",
    "Delete the named save '%1'? This cannot be undone from the game menu.":
        "Удалить сохранение «%1»? Отменить это действие из игры нельзя.",
    "The save could not be deleted: %1": "Не удалось удалить сохранение: %1",
    "The saved game could not be loaded. The file was left untouched.":
        "Не удалось загрузить игру. Файл остался нетронутым.",
    "A save named '%1' already exists. Replace it? The previous file will be kept as a backup.":
        "Сохранение «%1» уже существует. Заменить его? Предыдущая версия останется в резервной копии.",
    "Restore this emergency checkpoint as the current autosave? The existing autosave will be kept as a backup.":
        "Восстановить эту аварийную точку как текущее автосохранение? Нынешнее автосохранение останется в резервной копии.",
    "Start a new game? The current autosave will be replaced when new progress is saved.":
        "Начать новую игру? Текущее автосохранение будет заменено после сохранения нового прогресса.",
    "The checkpoint was loaded, but the current autosave could not be replaced: %1":
        "Точка восстановления загружена, но заменить текущее автосохранение не удалось: %1",
}


for number in range(1, 10):
    TRANSLATIONS[f"{number} Skull" if number == 1 else f"{number} Skulls"] = f"{number} Черепов"
    TRANSLATIONS[f"{number} Sword" if number == 1 else f"{number} Swords"] = f"{number} Клинков"
    TRANSLATIONS[f"{number} Number" if number == 1 else f"{number} Numbers"] = f"{number} Циферок"

for stat in ("Attack", "Defense", "Loyalty", "Missile"):
    russian = {"Attack": "Атака", "Defense": "Защита", "Loyalty": "Верность", "Missile": "Стрельба"}[stat]
    TRANSLATIONS[f"+%1 {stat}"] = f"+%1 {russian}"
    TRANSLATIONS[f"-%1 {stat}"] = f"-%1 {russian}"

TRANSLATIONS.update({
    "+%1 Attack, +%2 Max Loyalty": "+%1 Атака, +%2 Макс. Верность",
    "+%1 Attack or +%2 Missile or +%3 Max Loyalty":
        "+%1 Атака или +%2 Стрельба или +%3 Макс. Верность",
    "+%1 Attack, +%2 Missile, -%3 Max Loyalty":
        "+%1 Атака, +%2 Стрельба, -%3 Макс. Верность",
    "+%1 Loyalty, up to max": "+%1 Верность, не выше максимума",
    "-%1 Defense, -%2 Attack": "-%1 Защита, -%2 Атака",
})


def po_unquote(lines: list[str]) -> str:
    return "".join(ast.literal_eval(line[line.index('"'):]) for line in lines)


def po_quote(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace("\"", "\\\"")
    escaped = escaped.replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n")
    return f'msgstr "{escaped}"'


def update_block(block: str) -> tuple[str, bool]:
    lines = block.splitlines()
    msgid_index = next((i for i, line in enumerate(lines) if line.startswith("msgid ")), None)
    if msgid_index is None:
        return block, False

    msgid_lines = [lines[msgid_index]]
    cursor = msgid_index + 1
    while cursor < len(lines) and lines[cursor].startswith('"'):
        msgid_lines.append(lines[cursor])
        cursor += 1
    msgid = po_unquote(msgid_lines)
    translation = TRANSLATIONS.get(msgid)
    if translation is None:
        return block, False

    msgstr_index = next((i for i, line in enumerate(lines) if line.startswith("msgstr ")), None)
    if msgstr_index is None:
        return block, False
    msgstr_end = msgstr_index + 1
    while msgstr_end < len(lines) and lines[msgstr_end].startswith('"'):
        msgstr_end += 1

    lines[msgstr_index:msgstr_end] = [po_quote(translation)]
    for index, line in enumerate(lines):
        if line.startswith("#,") and "fuzzy" in line:
            flags = [flag.strip() for flag in line[2:].split(",") if flag.strip() != "fuzzy"]
            lines[index] = "#, " + ", ".join(flags) if flags else ""
    lines = [line for line in lines if line]
    return "\n".join(lines), True


def main() -> int:
    catalog = Path(sys.argv[1] if len(sys.argv) > 1 else "themes/default/lang/ru.po")
    source = catalog.read_text(encoding="utf-8")
    blocks = re.split(r"\n{2,}", source)
    updated = 0
    output: list[str] = []
    for block in blocks:
        replaced, changed = update_block(block)
        output.append(replaced)
        updated += int(changed)
    catalog.write_text("\n\n".join(output).rstrip() + "\n", encoding="utf-8", newline="\n")
    print(f"updated {updated} Russian messages in {catalog}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
