#!/usr/bin/env python3
"""Apply the curated Russian catalog derived from the localized Rune War resources."""

from __future__ import annotations

import ast
import json
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
    "Battle Intel": "Данные Разведки",
    "Capture: %1%": "Захват: %1%",
    "Attackers remain: %1%": "Выживет Атакующих: %1%",
    "Outcome hidden on Hard": "Исход скрыт на сложной",
    "Known: %1 attackers / %2 visible guards": "Известно: %1 атак. / %2 защитн.",
    "Town loyalty: %1 - known forces only": "Верность земли: %1 · только видимые",
    "Blue: move  Orange: attack": "Голубой: ход  Оранжевый: атака",
    "Yellow: claimable": "Жёлтый: можно присоединить",
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
    "Action Rejected": "Действие отклонено",
    "That action is not available in the current phase.":
        "Это действие недоступно в текущей фазе.",
    "It is not your turn.": "Сейчас не ваш ход.",
    "That choice is no longer available.": "Этот выбор больше недоступен.",
    "Silence prevents this action.": "Тишина не позволяет выполнить это действие.",
    "Mana Fog prevents casting this turn.":
        "Дым Маны не позволяет колдовать в этот ход.",
    "You have already summoned or cast a spell this turn.":
        "В этот ход вы уже призвали существо или применили заклинание.",
    "That Mahjong call is not legal for the current rune.":
        "Этот вызов в Игре Рун нельзя сделать для текущей руны.",
    "That rune choice is no longer available.":
        "Эта руна больше недоступна для выбора.",
    "Select a valid territory.": "Выберите подходящую землю.",
    "Select a valid target.": "Выберите подходящую цель.",
    "The required runes are not available.": "Не хватает требуемых рун.",
    "This action requires %2 spell points; only %1 are available.":
        "Для этого действия нужно %2 очков магии; доступно только %1.",
    "There are not enough spell points for that action.":
        "Для этого действия не хватает очков магии.",
    "This claim requires %2 claim points; only %1 are available.":
        "Для захвата нужно %2 очков захвата; доступно только %1.",
    "There are not enough claim points for that territory.":
        "Для захвата этой земли не хватает очков захвата.",
    "Your army has no free summoning slot.":
        "В вашей армии нет свободного места для призыва.",
    "That unique creature is already on the map.":
        "Это уникальное существо уже находится на карте.",
    "The destination party is full.": "В целевом отряде нет свободного места.",
    "The selected creature is no longer available.":
        "Выбранное существо больше недоступно.",
    "The selected creature cannot move to that territory.":
        "Выбранное существо не может перейти на эту землю.",
    "That territory cannot be claimed now.": "Сейчас эту землю нельзя присвоить.",
    "There are no Adventure orders to undo.":
        "Нет приказов фазы движения, которые можно отменить.",
    "There is no battle choice awaiting input.":
        "Сейчас нет ожидающего решения в бою.",
    "That battle choice is no longer legal.": "Этот выбор в бою больше недоступен.",
    "The game rejected that action.": "Игра отклонила это действие.",
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
    "Window Size": "Размер Окна",
    "Windowed": "В Окне",
    "Fullscreen": "Полный Экран",
    "ON": "ВКЛ",
    "OFF": "ВЫКЛ",
    "Changes apply immediately": "Изменения применяются сразу",
    "LEFT / RIGHT TO ADJUST": "← / → — ГРОМКОСТЬ",
    "Could not save settings: %1": "Не удалось сохранить настройки: %1",
    "Could not switch display mode.": "Не удалось переключить режим экрана.",
    "Could not resize the game window.": "Не удалось изменить размер окна игры.",
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


# Long lore entries are keyed by a stable, unique prefix.  This keeps the
# curated catalogue readable while still replacing the complete gettext
# message (whose English source text can span several paragraphs).
PREFIX_TRANSLATIONS: dict[str, str] = {
    # Creature specialities.
    "Creatures employing the ability to swarm":
        "Существа со способностью «Рой» могут атаковать весь отряд, а не только лидера, подавляя противника числом.",
    "This ability allows creatures of the same type to link together":
        "Эта способность позволяет существам одного типа объединяться в бою: каждое из них получает +1 к Атаке и +1 к Защите вместо обычного +1 к Атаке за поддержку.",
    "Invisible creatures may move across the isle":
        "Невидимые существа могут перемещаться по острову незамеченными и появляться лишь в бою. Сюрприз!",
    "Regenerating creatures are restored to maximum loyalty":
        "Существа с Регенерацией восстанавливают Верность до максимума в начале фазы карты.",
    "The presence of the possessor of this ability sends even the bravest":
        "Присутствие обладателя этой способности повергает в ужас даже храбрейших: все сражающиеся с ним теряют 2 Верности.",
    "Those possessing Major Magic Resistence":
        "Обладатели Высшего Сопротивления Магии с вероятностью %%{percent} игнорируют направленные на них заклинания любого колдуна.",
    "This fearsome ability gives the possessor a %25 chance":
        "Эта грозная способность даёт 25% шанс нанести Мощный Удар. Успешный удар повышает Атаку на 3 и гарантирует не менее 1 урона независимо от Защиты противника.",
    "This fearsome ability gives the possessor a 25% chance":
        "Эта грозная способность даёт 25% шанс нанести Мощный Удар. Успешный удар повышает Атаку на 3 и гарантирует не менее 1 урона независимо от Защиты противника.",
    "This ability allows the unit to teleport into any friendly territory":
        "Эта способность позволяет существу через врата переместиться в любую дружественную землю с Кругом Призыва.",
    "Those who possess this ability and lead their party into combat":
        "Если обладатель этой способности ведёт атакующий отряд в бой, существа пропускают залп, но получают Первый Удар в ближнем бою. Для обороняющихся отрядов способность не действует.",
    "Those blessed with this ability can see invisible units":
        "Обладатель этой способности видит невидимые отряды на соседних землях.",
    "Creatures with this ability can avoid the effects of missile fire":
        "Существа с этой способностью невосприимчивы к стрелковому огню в бою.",
    "Shannahan heals 2 loyalty before movement phase":
        "Шаннаган восстанавливает 2 Верности перед фазой движения.",
    "The fire shield reduces the loyalty of any creature":
        "Огненный Щит отнимает 1 Верность у любого существа, ударившего его владельца в ближнем бою.",

    # Creatures.
    "\tFoot soldiers of the skull summoning chain":
        "\tОрда Скелетов — пехота царства Черепов и надёжная поддержка. Благодаря численности она может нападать Роем, атакуя каждого участника вражеского отряда.",
    "\tShadows are a useful staple in the skull realm":
        "\tГнильё — полезная основа армии Черепов. Способности к Слиянию и Невидимости заметно усиливают его в атаке.",
    "\tA valued member of the skull summoning realm, the Durlock":
        "\tДрайдер — ценный воин царства Черепов. Его стрелковый удар и крепкая Атака служат хорошей основой сильного отряда.",
    "\tWraiths provide the most bang for the buck":
        "\tЧучело исключительно выгодно для царства Черепов. Регенерация делает его живучим и позволяет расправляться со слабыми и средними противниками и землями.",
    "\tThe Knight Templar is a solid low range summoning":
        "\tОтряд Рыцарей — надёжный недорогой призыв. Благодаря Мощному Удару он способен справиться со многими слабыми и средними противниками.",
    "\tExtremely resilient, the minotaur":
        "\tМинотавр чрезвычайно живуч и станет отличным выбором для призывателя: за умеренную цену он предлагает уверенные боевые характеристики.",
    "\tThe adventure party is a wise magical investment":
        "\tПриключенцы — разумное вложение магии. Они сочетают мощную наземную и воздушную атаку с крепкой защитой, а способность видеть невидимое может разрушить планы врага.",
    "\tThis high range creature is strong in all facets of combat":
        "\tПламенник силён во всех видах боя, как и многие существа Мечей. За мощным начальным выстрелом следует серьёзная атака в ближнем бою.",
    "\tThe sand wraith is an exceptional strategic unit":
        "\tДух Пустыни — ценное стратегическое существо благодаря Невидимости. Добавьте одного-двух духов к бойцу среднего уровня — и самоуверенные великаны начнут падать.",
    "\tWith its exceptional resilience and regeneration, the stone golem":
        "\tКаменный Голем очень живуч и владеет Регенерацией, поэтому отлично подходит на роль недорогого существа поддержки.",
    "\tThe thunder bird introduces a ranged attack":
        "\tПтица Молний приносит стрелковую атаку в царство Чисел. В сочетании с хорошей Атакой и Верностью это делает её сильным существом среднего уровня.",
    "\tThis fearsome serpent like beast has no visible weaknesses":
        "\tУ этого грозного змееподобного зверя нет явных слабостей. Отсутствие собственной Стрельбы компенсируется невосприимчивостью к снарядам и внушительными остальными характеристиками.",
    "\tA wizard must draw upon the power of the winds to summon a tornado":
        "\tЧтобы призвать Торнадо, колдун обращается к силе ветров. Это крепкое существо среднего уровня с ровными боевыми характеристиками и невосприимчивостью к стрелковым атакам.",
    "\tThe griffon is a credible strategic unit":
        "\tГрифон — полезное стратегическое существо. Слияние, способность видеть невидимое и достойные боевые качества делают его хорошим недорогим бойцом, а высокая подвижность — отличным разведчиком.",
    "\tChameleons are stealthy archers":
        "\tХамелеоны — скрытные стрелки из племени ящеров. Живучесть и верность Зиагу помогают им быстро восстанавливаться. Это единственное невидимое существо со Стрельбой.",
    "\tThe invisible undead knight is a formidable adversary":
        "\tНевидимый рыцарь-нежить опасен для любого существа среднего уровня. Усиленный дополнительными чарами, Килор становится грозным противником для любой армии.",
    "\tThe Demon Lord known as Maz'Ra":
        "\tВладыка Демонов Маз'Ра, возможно, самое страшное существо, доступное призывателям. Его сокрушительная Атака и устрашающий Взрыв Огня позволяют в одиночку уничтожать сильные армии.",
    "\tCarol is a versatile addition to any summoner":
        "\tВеликий Профосс — универсальное подкрепление. Его кулон открывает врата к дружественным Кругам Призыва и даёт ведомой им атакующей армии Первый Удар.",
    "\tKing Drago is the jewel of the realm of swords":
        "\tКороль Уго — жемчужина царства Мечей. Его мастерство войны дополняют Сопротивление Магии и грозный Мощный Удар. Это превосходная боевая машина, которую, увы, трудно лечить.",
    "\tA hauntingly gorgeous individual, Learra's hypnotic song":
        "\tЗавораживающе прекрасная Сирена Ниеннах гипнотической песней сковывает врагов, когда ведёт атакующий отряд, и получает Первый Удар. Её высокая Атака делает этот удар особенно опасным.",
    "\tShanahan the Cloud Giant is the pinnacle":
        "\tОблачный великан Шаннаган — вершина царства Чисел. Он способен сокрушить почти любого врага в одиночку или присоединиться к отряду и сеять хаос по всему острову.",
    "\tThe White Dragon is extremely resilient":
        "\tБелый Дракон от природы чрезвычайно живуч и верен. Он позволяет призывателю получить руну Чисел, а ровные боевые характеристики делают его первоклассным бойцом.",
    "\tThe Green Dragon is a very hostile":
        "\tЗелёный Дракон — свирепый и агрессивный зверь. Отсутствие Стрельбы компенсируют огромная Атака и высокая Верность; пока он в игре, хозяин может получить руну Мечей.",
    "\tThe Red Dragon has a particular affinity":
        "\tКрасный Дракон связан с царством Черепов и позволяет призывателю получить руну Черепа. Мощное дыхание дальнего боя и крепкая Защита делают его опасным слугой.",
    "\tThe Fire Elemental is a particularly volatile creature":
        "\tЭлементаль Огня крайне непредсказуем. Он сочетает сильные ближнюю и стрелковую атаки, но его главный дар — заклинание «Случайная Ставка».",
    "\tBorn from the ground beneath, the Earth Elemental":
        "\tРождённый в глубинах земли Элементаль Земли — надёжная боевая сила. Его владелец получает заклинание «Ясновидение», позволяющее заглянуть в руки противников.",
    "\tThe Air Elemental proves to be the greatest combatant":
        "\tЭлементаль Эфира — сильнейший боец среди стихий. Призвавший его колдун получает «Тишину» — заклинание, способное серьёзно досадить любому сопернику.",
    "\tThe Water Elemental is a powerful force":
        "\tЭлементаль Воды силён и на острове, и в Игре Рун. Его командир получает «Дым Маны» — весьма неприятное стратегическое заклинание.",
    "\tThis creature leads its category on the battlefield":
        "\tДжагернаут — один из лучших бойцов своего класса. Он сочетает огромную силу, живучесть и высокую подвижность и полностью оправдывает затраты на призыв.",

    # Spells.
    "\tA cloud of billowing smoke surrounds one enemy creature":
        "\tКлубы Тумана окутывают одно вражеское существо и ограничивают его видимость, уменьшая или полностью лишая его Стрельбы.",
    "\tThe caster draws upon the power of the demons":
        "\tКолдун обращается к силе демонов, чтобы повысить Верность одного из своих существ.",
    "\tThe summoner directs the face of fear":
        "\tПризыватель обрушивает воплощённый страх на вражеский отряд, повергая его в панику и снижая Верность.",
    "\tAnother mind altering spell, paralyzing fear":
        "\tПарализующий страх сковывает разум одного вражеского существа и лишает его возможности двигаться в течение одного хода.",
    "\tWith this spell the summoner is able to harness the power of the skull realm":
        "\tПризыватель использует силу царства Черепов, чтобы подавить волю одного вражеского существа и снизить его боевые характеристики.",
    "\tAn intense bloodlust flows through the veins":
        "\tЖажда крови наполняет зачарованное существо и делает его гораздо свирепее в атаке.",
    "\tThis spell sharpens the vision":
        "\tЗаклинание обостряет зрение дружественного стрелка и позволяет ему точнее наносить дальние удары.",
    "\tThe caster of this spell is able to conjure a shield of energy":
        "\tКолдун окружает одно дружественное существо энергетическим щитом, ослабляющим стрелковый огонь существ и территорий.",
    "\tSwirling winds surround the target":
        "\tВихревые ветры поднимают вокруг цели пыль и обломки. Существо теряет концентрацию и хуже защищается.",
    "\tThe target of this spell becomes inspired by visions":
        "\tВидения великих воинов, сокрушающих врагов, вдохновляют цель и значительно повышают её боевую мощь.",
    "\tThe waters of a great mystical fountain":
        "\tВоды Мистического Ручья даруют выбранному существу случайное усиление: Атаку, Верность или Стрельбу.",
    "\tWhen a friendly creature is enchanted with this spell":
        "\tЗачарованное дружественное существо рвётся в бой и жертвует Верностью ради усиления Атаки.",
    "\tExtremely bright lights appear":
        "\tЯркие огни танцуют вокруг цели, отвлекают её от боя и снижают способность защищаться.",
    "\tThe magic of this enchantment consumes its target":
        "\tМагическая аура придаёт цели уверенность в собственных силах и повышает её Защиту.",
    "\tAn excellent utility spell, teleport":
        "\tТелепорт открывает врата перемещения и переносит одно дружественное существо на любую союзную землю.",
    "\tRemoves all enchantments from the target creature":
        "\tСнимает с выбранного существа все наложенные чары, не затрагивая его врождённые способности.",
    "\tThe caster of this spell summons the power of the skies":
        "\tКолдун призывает силу небес и поражает вражеское существо Шаровой Молнией. Громко и эффектно!",
    "\tAn important supplementary spell, healing":
        "\tЛечение восстанавливает дружественному существу 2 Верности, но не выше его максимума.",
    "\tThe most destructive spell in the lands, hellblast":
        "\tВзрыв Огня — самое разрушительное заклинание острова. Оно окружает выбранную вражескую армию обжигающим огненным штормом и наносит тяжёлый урон.",
    "Removes all enchantments from every creature":
        "Снимает все чары со всех существ на выбранной земле, не затрагивая их врождённые способности.",
    "A mana-dampening fog covers every wizard":
        "Дым Маны окутывает всех колдунов и на три хода запрещает обычное применение заклинаний и призыв существ.",
    "Forces the targeted wizard to discard a random rune":
        "Заставляет выбранного колдуна сбросить случайную руну в начале следующего хода.",
    "Reveals the targeted opponent's concealed runes":
        "На пять ходов открывает заклинателю скрытые руны выбранного противника.",
    "Prevents the targeted wizard from calling Chao":
        "На три хода запрещает выбранному колдуну объявлять Чоу, Панг, Конг и Маджонг, применять заклинания и призывать существ.",
    "Heals +2 loyalty before movement phase":
        "Восстанавливает 2 Верности перед фазой движения.",
    "The ranger attack speciality":
        "Существо участвует в начальном стрелковом залпе.",
}


PREFIX_TRANSLATIONS.update({
    # Wizards. Clan names in these restored biographies follow the current
    # colour identity used throughout Four Winds Reborn.
    "\tThe spawn of all that is evil and callous": (
        "\tВоплощение всего злого и бессердечного — некромант по имени %1. На Острове Четырёх Ветров нет существа безжалостнее этого лишённого чувств колдуна. Печаль, страх, гнев, радость, зависть и жадность не тревожат его и потому делают самым сосредоточенным и пугающим из всех магов. Смерть и разложение питают его существование и заменяют ему совесть. В тёмных глазах %1 будто видны измученные души тех, кто пересёк ему дорогу. Говорят, встретить %1 — значит встретить саму смерть."
        "\n\tСущность %1 неразрывно связана с магией царства Черепов. Смерть служит ему источником магии и самой тканью его бытия. Неземная аура %1 пульсирует тёмным пламенем смерти; это его единственная связь с Островом Четырёх Ветров, поэтому для призыва и чар он пользуется главным образом магией Черепов. Но близость к смерти роднит его и с самыми разрушительными существами острова. Красный Дракон, Элементаль Огня и владыка демонов Маз'Ра уже исполняли его волю и наверняка будут призваны вновь."
        "\n\t%1 — настоящий наёмник, не принадлежащий ни к одному из народов, борющихся за остров. Он выбирает союзников лишь ради собственной выгоды и укрепления своей зловещей магической сущности. Только Красный и Фиолетовый кланы оказались достаточно смелы, чтобы воспользоваться им как орудием в прежних Войнах Рун. В новой войне %1 снова поддержит один из них в надежде добиться полного господства и умножить свою дьявольскую силу."
        "\n\tПроисхождение %1 остаётся загадкой. Он несомненно жив, но каждое его появление окружено легендами. Все народы острова спорят о причине его существования. Одни считают его владыкой, бросившим вызов богам бесконечными убийствами: после его странствий земли остаются мрачными и опустошёнными. Другие видят в нём воплощение мирового зла. Все сходятся лишь в одном: %1 — самое странное и страшное существо острова. Чтобы победить его, колдуну потребуются железная воля и каменное сердце — %1 не знает пощады."
    ),
    "\t%1 of the Kartha stands by the chivalric code": (
        "\t%1 из Жёлтого клана следует рыцарскому кодексу чести. Всё, что он делает, служит благу его народа. Никто на острове не сравнится с его верой и преданностью соотечественникам. %1 убеждён, что терпение и добродетель приносят великую награду. Его нравственные принципы порождены не только религией — они вплетены в саму его магическую природу. Упорство позволило %1 обуздать ауру и подняться над обыденным существованием. Поэтому он не просто рыцарь или церковный крестоносец: магия стала продолжением его клинка и крепнет вместе с мастерством мечника и чародея света."
        "\n\tГоды изучения рыцарских искусств открыли %1 магию, скрытую глубоко в его душе. Терпение, свойственное Жёлтому клану, благодаря почти машинной целеустремлённости стало его инстинктом. Магическая природа %1 заключена в силе Меча, и все его призывы и чары связаны с этим царством. Он часто обращается за помощью к прославленным героям — Великому Профоссу и Королю Уго. Они разделяют святые идеалы, которыми он дорожит. %1 черпает силу в призванных союзниках и потому редко появляется один."
        "\n\tДругие народы Острова Четырёх Ветров часто неверно понимают %1, считая мягкосердечным защитником несчастных. В Войне Рун такая ошибка опасна. Соперники могут ожидать, что он удовольствуется скромной победой, но %1 стремится к полному господству. Безграничная преданность народу заставляет его добиваться сокрушительного результата и вести своих людей к славе. Сильный разум и непревзойдённое терпение делают %1 достойным противником."
    ),
    "\tThere is a point in the development of a powerful wizard": (
        "\tВ обучении сильного колдуна наступает миг, когда наставник понимает: перед ним настоящий талант. Ученица Киерака %1 так и не достигла подобной вершины. Киерак рано заметил её способности и считает её умелой, хотя и не блистательной волшебницей — из тех, кого прежде не ожидали увидеть в Башне Магов среди участников Войны Рун."
        "\n\tНикто не знает, когда именно %1 заняла законное место среди колдунов Фиолетового клана и стала претенденткой на роль его избранницы. Её главное достоинство в том, что соперники постоянно её недооценивают. Тактика и заклинания %1 редко выглядят эффектно, но итог неизменно оказывается впечатляющим."
        "\n\tКак урождённая представительница Фиолетового клана, %1 способна дышать и воздухом, и под водой. Водная стихия дарит ей свободу: остров изрезан каналами и реками, по которым она перемещается тихо и незаметно. В скрытности %1 не уступает самому Киераку, хотя об этом почти никому не известно."
    ),
    "\tAccording to %1, conditions for his victory": (
        "\tПо мнению %1, обстоятельства как никогда благоприятствуют его победе. После неудач прежних вождей в прошлых Войнах Рун другие народы почти перестали считаться с Бирюзовым кланом. %1 намерен удивить соперников. Он изучил прежние поражения и решил, что лишь постоянное самосовершенствование способно вернуть униженному народу уважение. В поисках редких магических знаний %1 обошёл все уголки острова. Он сознательно отказался от наставника, не найдя достойного учителя среди прежних вождей и колдунов. Чутко ощущая магические приливы собственной души и врождённую силу своего народа, %1 научился превращать надежды бирюзовых в оружие мести всем, кто называл их слабыми."
        "\n\tОсобенно высоко %1 ценит ту сторону войны, которая способна сделать его опаснейшим участником состязания: тактику. Благодаря упорным занятиям, размышлениям и магии он стал мастером стратегического применения сил. Бесчисленные повторения научили его молниеносно оценивать обстановку и инстинктивно приспосабливаться к любому сценарию. В интеллектуальной битве %1 рассчитывает превзойти всех соперников, а его знаменитый монокль открывает каждое невидимое существо на карте."
        "\n\tМагия %1 необычайно гибка. Хотя его призывы главным образом связаны с царством Черепов, он владеет всем спектром тактических чар и усиливает существ множеством дополнительных заклинаний. %1 никогда не копит доставшуюся ему силу: он постоянно расходует её, укрепляя армию. Это эффектный боец, предпочитающий скорость и тонкий расчёт грубой мощи. Глубокое знание чар помогает ему не позволять врагам окружить или подавить себя."
        "\n\tПоход %1 за знаниями вернул надежду его народу. Он чувствует воодушевление бирюзовых в потоке магии, проходящем сквозь его кровь. Способность обратить надежды клана в силу позволит %1 сразиться на равных с любым колдуном — особенно с теми, кто всё ещё считает Бирюзовый клан досадной мелочью."
    ),
    "\t%1 is another foreigner to the Isle of Four Winds": (
        "\t%1 — ещё одна чужеземка на Острове Четырёх Ветров, человек. Ненависть к силам зла привела её сюда. Долгие годы она шла по следу одного колдуна, мечтая встретить его и изгнать из мира живых. Имя этого врага — Орачин, источник бури, бушующей в её душе. Неустанные поиски наконец открыли %1 убежище заклятого противника, и теперь она готова сразиться с ним. В новой Войне Рун она примет предложение любого клана, лишь бы выступить против Орачина. Служа нанимателям, %1 не забывает своей истинной цели — мести."
        "\n\tКогда-то %1 была тихой женщиной и жила в маленьком земледельческом поселении далеко от острова. Она понемногу изучала магию, но не располагала знаниями и средствами, чтобы раскрыть настоящий дар. Её жизнь принадлежала семье и городку у великой реки — пока не настал роковой день. Небо потемнело, страшная буря сотрясла дома, а вслед за ней пришла армия мертвецов и демонов, убивавшая всё живое и обращавшая его в проклятую нежить. Во главе шёл лишённый чувств Орачин-некромант. За несколько минут поселение погибло, и Орачин исчез так же внезапно, как появился. %1 спаслась в маленькой пещере на речном берегу. Когда вернулись солнце и синее небо, она увидела лишь руины и мёртвую тишину. Отбросив прежнюю чистоту, %1 посвятила себя мести. Единственным путём стали призыв и чары. Она обратилась к единственному известному в тех землях источнику магии — Владыке Демонов — и заключила с ним договор: душа в обмен на знание. %1 ненавидела свой выбор, но гнев затмил рассудок. Она переродилась дитём огня."
        "\n\tС тех пор её путь — суровое обучение и магические опыты. %1 взращивала пламя, посеянное демоном, и ритуальными медитациями питала огонь знания. Маленькая искра превратилась во Взрыв Огня, способный поглощать целые армии. Отвращение к магии Черепов Орачина направило её силу к Мечам и Числам. Так %1 стала грозной призывательницей и повелительницей чар двух царств — силой, с которой придётся считаться в новой Войне Рун."
    ),
    "\t\"The absolute master of summoning\"": (
        "\t«Абсолютный мастер призыва» — лучшее определение для %1. Благодаря глубочайшему знанию главных Кругов Призыва его считают старейшим колдуном острова. %1 окружён тайнами почти так же, как Орачин, но славу ему принесли не мучения и разрушение, а умение изменять собственное тело. Отследить его прежние обличья и странствия почти невозможно, поэтому возраст и происхождение %1 остаются предметом догадок."
        "\n\tИмя %1 звучит во множестве легенд. Учёные и жители сходятся в том, что он участвовал в нескольких Войнах Рун на стороне Фиолетового и Бирюзового кланов, которые привлекают его своей чуждостью. В прошлых состязаниях %1 поражал врагов способностью призывать могучих существ из всех главных царств — Черепов, Мечей и Чисел. Комбинируя их силы, он создавал смертоносные армии и заставлял соперников гадать, как один колдун мог овладеть столь разными школами."
        "\n\tПредполагали, что %1 учился у мастеров каждого царства, но ни один из них не признавался, что наставлял его. Ответ появился, когда раскрылась его природа Изменяющего Плоть: великие учителя передавали ему знания, не понимая, кому на самом деле преподают."
        "\n\t%1 безразлично, какими средствами добывать знания. Он использует всё доступное, чтобы достичь полного мастерства призыва и принести победу тем, кто наймёт его на Войну Рун."
        "\n\tНенасытная жажда и редчайшая способность к обучению не оставили %1 места среди одного клана. Единственная цель его жизни — призыв. Пока в мире остаётся знание, он будет искать его любыми способами. Посвятив себя всем школам призыва, %1 стал самым прославленным колдуном Войн Рун. Он презирает «пустячную» дополнительную магию и добивается победы грубой силой, вызывая самых могучих и жестоких существ. Поэтому %1 снова считается главным фаворитом — если, конечно, решит участвовать. Это покажет лишь время."
    ),
    "The absolute master of summoning ": (
        "\t«Абсолютный мастер призыва» — лучшее определение для %1. Благодаря глубочайшему знанию главных Кругов Призыва его считают старейшим колдуном острова. %1 окружён тайнами почти так же, как Орачин, но славу ему принесли не мучения и разрушение, а умение изменять собственное тело. Отследить его прежние обличья и странствия почти невозможно, поэтому возраст и происхождение %1 остаются предметом догадок."
        "\n\tИмя %1 звучит во множестве легенд. Учёные и жители сходятся в том, что он участвовал в нескольких Войнах Рун на стороне Фиолетового и Бирюзового кланов, которые привлекают его своей чуждостью. В прошлых состязаниях %1 поражал врагов способностью призывать могучих существ из всех главных царств — Черепов, Мечей и Чисел. Комбинируя их силы, он создавал смертоносные армии и заставлял соперников гадать, как один колдун мог овладеть столь разными школами."
        "\n\tПредполагали, что %1 учился у мастеров каждого царства, но ни один из них не признавался, что наставлял его. Ответ появился, когда раскрылась его природа Изменяющего Плоть: великие учителя передавали ему знания, не понимая, кому на самом деле преподают."
        "\n\t%1 безразлично, какими средствами добывать знания. Он использует всё доступное, чтобы достичь полного мастерства призыва и принести победу тем, кто наймёт его на Войну Рун."
        "\n\tНенасытная жажда и редчайшая способность к обучению не оставили %1 места среди одного клана. Единственная цель его жизни — призыв. Пока в мире остаётся знание, он будет искать его любыми способами. Посвятив себя всем школам призыва, %1 стал самым прославленным колдуном Войн Рун. Он презирает «пустячную» дополнительную магию и добивается победы грубой силой, вызывая самых могучих и жестоких существ. Поэтому %1 снова считается главным фаворитом — если, конечно, решит участвовать. Это покажет лишь время."
    ),
    "\tAlthough %1 is a new face on the Isle of Four Winds": (
        "\tХотя %1 недавно появился на Острове Четырёх Ветров, его участия в Войне Рун ждут с большим интересом. Он не принадлежит к местным народам и представляет редкий вид фей. Подобно Нукрусу, %1 не связан с отдельным магическим царством, зато обладает огромным талантом призывателя. Он снискал расположение драконов и способен вызывать все разновидности этих разрушительных созданий. Кроме того, %1 раскрыл тайну Джагернаута и наверняка призовёт его, чтобы посеять полный хаос."
        "\n\tКак истинный наёмник, %1 предложит услуги любому клану. Рассказы о его невероятной силе разошлись по всему острову и привлекли внимание всех правителей. По натуре бард, %1 устроил настоящее представление: путешествовал по землям, развлекая жителей песнями и стихами о собственном могуществе. Народы полюбили зачарованного артиста и захотели видеть его участником Войны Рун, чем резко подняли его ценность для вождей. %1 сможет выбирать из множества предложений. Его поддерживают простые жители всех народов — те, кто готов разделить с ним кружку и смех. Других колдунов боятся за их странности, а %1 любят за весёлый нрав и обаяние. Он намерен питаться восторгом толпы: великому артисту нужна большая публика."
        "\n\tСлабость %1 — отсутствие собственной школы дополнительных заклинаний. Станет ли это проблемой, зависит от выпавших рун. Дерзкому и убедительному барду достаточно обратиться к драконам как к источнику победы. Уже поэтому за %1 стоит внимательно следить."
    ),
    "\tTouted as the elite Rune War contestant": (
        "\t%1 считается элитным участником Войны Рун и представляет Красный клан. Ветеран прежних состязаний, он прежде всего мастер самой Игры Рун и полагается на умение играть сильнее, чем на заключённую в рунах магию. %1 — универсальный, но ограниченный призыватель: он способен вызывать стихий и младших существ, однако не связан ни с одним главным царством и потому не может собрать обычный могучий легион. Это заставило его избрать путь, непохожий на стратегии большинства колдунов."
        "\n\tТактика %1 чрезвычайно эффективна и выводит соперников из себя. Понимая, что не сможет подавить врагов числом призванных существ, он сосредоточился на подвижности и защите земель. Искусная игра позволяет %1 быстро расширять владения, а Телепорт переносит существ для внезапных ударов по землям, которые неосторожные враги оставили уязвимыми. Его магическая природа не требует делить внимание между тактикой на карте и Игрой Рун, поэтому %1 может сокрушать противников продвинутой стратегией за столом. Спокойный и рассудительный Красный клан высоко ценит такой подход и много лет доверяет своему избраннику. Но права на ошибку у %1 почти нет: гордый народ не простит провала и расторгнет договор, если победа окажется под угрозой."
        "\n\t%1 нравится давление, связанное с ролью красного колдуна. Этот народ высоко ценит себя, быстроту мысли и умение решать сложные задачи. Соперники привыкли видеть, как %1 ослепляет их блестящей игрой и решениями, будто забавляясь. Он держится с самоуверенной ухмылкой, зная, что прежде мало кто мог сравниться с его мастерством. %1 с нетерпением ждёт новой Войны Рун, чтобы снова проучить ветеранов и поставить новичков в неловкое положение."
    ),
    "%1's affiliation to a clan is torn": (
        "%1 разрывается между двумя кланами, поскольку не имеет единственного наследия. Он родился на острове, но в его жилах течёт кровь двух народов: мать была старейшиной Красного клана, отец — преданным крестоносцем Жёлтого. %1 унаследовал острый ум красных и неукротимую целеустремлённость жёлтых, став редчайшим дарованием, словно созданным для магии и Войн Рун. Правители обоих кланов поняли это и начали бороться за право распоряжаться им. Желая покоя и возможности изучать призыв и чары, %1 покинул родной нейтральный городок между двумя землями. Его уход заставил вождей искать страшное решение: каждый клан решил, что после смерти одного родителя %1 выберет народ выжившего. Мать и отца заставили сойтись в учебной Войне Рун, проигравшего ожидала казнь. Находившийся далеко в обучении %1 ничего не знал. Родители приняли собственное решение: вместе выпили яд и завершили спор трагедией. Истину до сих пор скрывают от %1, а два народа договорились честно делить право приглашать его на Войну Рун."
        "\n\tМагическая аура %1 сформировалась после ухода из дома. Он нашёл убежище в горах и слился с чистым воздухом вершин, научившись чувствовать и направлять заключённую в нём магию. Не принадлежа отдельному царству, %1 связывает все призывы с полётом и ветрами. Он самостоятельно стал мастером воздуха и вызывает таких существ, как Ки-Рин, Грифон и Элементаль Эфира. Дополнительные чары выросли из наследия отца: жёлтая преданность Мечу жила в его крови, и %1 позволил этой силе стать основой своих заклинаний."
        "\n\tНикто не знает, какой клан %1 представит в новой Войне Рун. Незнание судьбы родителей пока сохраняет его связь с обоими народами, и он должен лишь выбрать одного из них. Но если %1 узнает правду об их гибели, никто не способен предсказать, во что превратится закипающая в нём ярость."
    ),
})


RUNTIME_DESCRIPTION_FILES = (
    "avatars.json",
    "creatures.json",
    "spells.json",
    "specials.json",
    "abilities.json",
)


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


def po_field(name: str, value: str) -> str:
    return po_quote(value).replace("msgstr", name, 1)


def normalized_lookup(value: str) -> str:
    value = re.sub(r"\[color:[^\]]+\]", "", value)
    return value.lstrip("\t")


def translation_for(msgid: str) -> str | None:
    translation = TRANSLATIONS.get(msgid)
    if translation is not None:
        return translation

    normalized = normalized_lookup(msgid)
    return next(
        (
            value
            for prefix, value in PREFIX_TRANSLATIONS.items()
            if normalized.startswith(normalized_lookup(prefix))
        ),
        None,
    )


def runtime_descriptions() -> dict[str, list[str]]:
    root = Path(__file__).resolve().parents[1]
    data_dir = root / "themes" / "default" / "json" / "gamedata"
    messages: dict[str, list[str]] = {}

    for filename in RUNTIME_DESCRIPTION_FILES:
        path = data_dir / filename
        payload = json.loads(path.read_text(encoding="utf-8"))
        containers = payload if isinstance(payload, list) else payload.values()
        rows = containers if isinstance(payload, list) else (
            row for container in containers if isinstance(container, list) for row in container
        )
        reference = path.relative_to(root).as_posix()
        for row in rows:
            if not isinstance(row, dict):
                continue
            description = row.get("description", "")
            if description:
                messages.setdefault(description, []).append(reference)

    return messages


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
    translation = translation_for(msgid)
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
    is_template = catalog.suffix == ".pot"
    updated = 0
    output: list[str] = []
    existing: set[str] = set()
    for block in blocks:
        lines = block.splitlines()
        msgid_index = next((i for i, line in enumerate(lines) if line.startswith("msgid ")), None)
        if msgid_index is not None:
            msgid_lines = [lines[msgid_index]]
            cursor = msgid_index + 1
            while cursor < len(lines) and lines[cursor].startswith('"'):
                msgid_lines.append(lines[cursor])
                cursor += 1
            existing.add(po_unquote(msgid_lines))

        replaced, changed = (block, False) if is_template else update_block(block)
        output.append(replaced)
        updated += int(changed)

    added = 0
    for msgid, references in runtime_descriptions().items():
        if msgid in existing:
            continue
        translation = "" if is_template else (translation_for(msgid) or "")
        output.append(
            "#: " + " ".join(sorted(set(references))) + "\n" +
            po_field("msgid", msgid) + "\n" +
            po_field("msgstr", translation)
        )
        existing.add(msgid)
        added += 1

    catalog.write_text("\n\n".join(output).rstrip() + "\n", encoding="utf-8", newline="\n")
    print(f"updated {updated} and added {added} runtime messages in {catalog}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
