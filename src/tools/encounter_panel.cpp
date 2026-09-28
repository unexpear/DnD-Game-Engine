/**
 * @file encounter_panel.cpp
 * @brief Encounter panel widget, click routing, and scene refresh.
 *
 * See encounter_panel.h. Clicks are queued on the mouse-down edge and applied
 * in EncounterPanel::Update, which runs later in the same frame.
 *
 * @author DND Engine Team
 * @date 2026-09-28
 * @version 0.1.0
 */

#include "tools/encounter_panel.h"
#include "tools/editor_settings.h"
#include "ui/ui_widget.h"
#include "ui/ui_renderer.h"
#include "core/input.h"
#include "core/log.h"
#include "game/dice.h"
#include <cstdio>
#include <string>
#include <vector>

namespace Dnd {

namespace {

/**
 * @brief One clickable control in the encounter view.
 */
struct EncounterCommand {
    enum class Type : u8 {
        Spawn,
        ToggleParty,
        Roll,
        Damage,
        Heal,
        Full,
        Start,
        Next,
        End,
        Remove,
        Select
    };

    Type type = Type::Spawn;
    const char* statId = nullptr;
    u32 memberId = 0;
    i32 amount = 0;
};

/**
 * @brief A button rectangle plus the command it emits.
 */
struct EncounterButton {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 w = 0.0f;
    f32 h = 0.0f;
    std::string caption;
    EncounterCommand command;
    b8 selected = false;
    b8 current = false;
};

constexpr f32 kFont = 8.0f;
constexpr f32 kBtnH = 18.0f;
constexpr f32 kRowH = 16.0f;
constexpr f32 kGap = 3.0f;

/**
 * @brief Draws a filled button and its caption.
 * @param button Button rectangle and caption.
 * @param fill Background color.
 */
void DrawButton(const EncounterButton& button, const Vec4& fill) {
    UIRenderer::DrawRect(button.x, button.y, button.w, button.h, fill);
    const f32 textW = static_cast<f32>(button.caption.size()) * kFont;
    const f32 textX = button.x + (button.w - textW) * 0.5f;
    const f32 textY = button.y + (button.h - kFont) * 0.5f;
    UIRenderer::DrawText(textX, textY, button.caption.c_str(), Vec4(0.95f, 0.95f, 0.97f, 1.0f), kFont);
}

/**
 * @brief Appends a button if it still fits in the content rect.
 * @param buttons Output list.
 * @param x Left edge.
 * @param y Top edge.
 * @param w Width.
 * @param h Height.
 * @param caption Label.
 * @param command Click command.
 * @param bottom Exclusive bottom of the content area.
 * @return The next y after this button, whether or not it fit.
 */
f32 PushButton(std::vector<EncounterButton>& buttons, f32 x, f32 y, f32 w, f32 h,
               const std::string& caption, const EncounterCommand& command, f32 bottom) {
    if (y + h <= bottom && w > 8.0f) {
        EncounterButton button;
        button.x = x;
        button.y = y;
        button.w = w;
        button.h = h;
        button.caption = caption;
        button.command = command;
        buttons.push_back(button);
    }
    return y + h;
}

} // namespace

/**
 * @brief Encounter panel body. Owned by the DockPanel as its content widget.
 */
class EncounterPanel::View : public UIWidget {
public:
    /**
     * @brief Constructs the view bound to a roster.
     * @param builder Roster the buttons edit. Must outlive the view.
     */
    explicit View(EncounterBuilder* builder)
        : UIWidget("EncounterView"), m_builder(builder) {}

    void Update(f32 deltaTime) override {
        DND_UNUSED(deltaTime);
        // DockManager does not deliver mouse-up to panel content, so the
        // click latch is cleared here once the button is released.
        if (!Input::IsMouseButtonDown(MouseButton::LEFT)) m_wasDown = false;
    }

    b8 HandleInput(f32 mouseX, f32 mouseY, b8 mouseDown) override {
        const b8 pressed = mouseDown && !m_wasDown;
        if (mouseDown) m_wasDown = true;
        if (!pressed || !m_builder) return false;

        std::vector<EncounterButton> buttons;
        Build(buttons);
        for (const EncounterButton& button : buttons) {
            UIRect rect(button.x, button.y, button.w, button.h);
            if (!rect.Contains(mouseX, mouseY)) continue;
            m_commands.push_back(button.command);
            return true;
        }
        return false;
    }

    void Render(const Mat4& projMatrix) override {
        DND_UNUSED(projMatrix);
        if (!m_visible || !m_builder) return;

        std::vector<EncounterButton> buttons;
        const f32 bottom = m_position.y + m_size.y;
        f32 y = Build(buttons);

        char status[160];
        const InitiativeRoll* turn = m_builder->CurrentTurn();
        if (turn) {
            std::snprintf(status, sizeof(status), "COMBAT  %d/%d  %s %d",
                          m_builder->TurnIndex() + 1,
                          static_cast<i32>(m_builder->Order().size()),
                          turn->label.c_str(), turn->total);
        } else if (!m_builder->Order().empty()) {
            std::snprintf(status, sizeof(status), "Prep  initiative ready (%d)",
                          static_cast<i32>(m_builder->Order().size()));
        } else {
            std::snprintf(status, sizeof(status), "Prep  add a monster, then Roll Init");
        }
        if (m_position.y + kFont <= bottom) {
            UIRenderer::DrawText(m_position.x, m_position.y, status,
                                 Vec4(0.92f, 0.86f, 0.55f, 1.0f), kFont);
        }

        for (const EncounterButton& button : buttons) {
            Vec4 fill(0.26f, 0.30f, 0.38f, 1.0f);
            if (button.command.type == EncounterCommand::Type::Select) {
                fill = button.selected ? Vec4(0.22f, 0.36f, 0.52f, 1.0f)
                                       : Vec4(0.16f, 0.17f, 0.20f, 1.0f);
                if (button.current) fill = Vec4(0.42f, 0.34f, 0.14f, 1.0f);
            } else if (button.command.type == EncounterCommand::Type::Start) {
                fill = Vec4(0.18f, 0.40f, 0.24f, 1.0f);
            } else if (button.command.type == EncounterCommand::Type::End ||
                       button.command.type == EncounterCommand::Type::Remove) {
                fill = Vec4(0.42f, 0.22f, 0.20f, 1.0f);
            }
            DrawButton(button, fill);
        }

        if (y + kFont <= bottom) {
            UIRenderer::DrawText(m_position.x, y, "Order",
                                 Vec4(0.75f, 0.78f, 0.85f, 1.0f), kFont);
            y += kRowH;
            if (m_builder->Order().empty()) {
                if (y + kFont <= bottom) {
                    UIRenderer::DrawText(m_position.x, y, "(not rolled)",
                                         Vec4(0.60f, 0.62f, 0.68f, 1.0f), kFont);
                }
            } else {
                const u32 currentId = turn ? turn->id : 0;
                i32 shown = 0;
                for (const InitiativeRoll& roll : m_builder->Order()) {
                    if (y + kFont > bottom || shown >= 8) break;
                    char line[96];
                    std::snprintf(line, sizeof(line), "%s%s %d",
                                  (m_builder->IsCombatActive() && roll.id == currentId) ? "> " : "  ",
                                  roll.label.c_str(), roll.total);
                    const Vec4 color = (m_builder->IsCombatActive() && roll.id == currentId)
                        ? Vec4(1.0f, 0.86f, 0.40f, 1.0f)
                        : Vec4(0.82f, 0.84f, 0.88f, 1.0f);
                    UIRenderer::DrawText(m_position.x, y, line, color, kFont);
                    y += kRowH;
                    ++shown;
                }
            }
        }
    }

    /**
     * @brief Takes the clicks queued since the last call.
     * @return Commands in click order.
     */
    std::vector<EncounterCommand> TakeCommands() {
        std::vector<EncounterCommand> out;
        out.swap(m_commands);
        return out;
    }

    /**
     * @brief Highlights a roster row.
     * @param memberId Selected roster id, or 0 for none.
     */
    void SetSelected(u32 memberId) { m_selected = memberId; }

private:
    /**
     * @brief Lays out every button from the current roster.
     * @param buttons Output. Cleared first.
     * @return Y coordinate just below the roster, where the order list starts.
     */
    f32 Build(std::vector<EncounterButton>& buttons) const {
        buttons.clear();
        const f32 left = m_position.x;
        const f32 width = m_size.x;
        const f32 bottom = m_position.y + m_size.y;
        f32 y = m_position.y + kRowH + kGap;
        if (!m_builder || width < 32.0f) return y;

        const f32 inner = width;
        const f32 third = (inner - kGap * 2.0f) / 3.0f;
        const MonsterStatBlock* goblin = EncounterBuilder::FindStat("goblin");
        const MonsterStatBlock* skeleton = EncounterBuilder::FindStat("skeleton");
        const MonsterStatBlock* bugbear = EncounterBuilder::FindStat("bugbear");
        auto spawnCmd = [](const char* id) {
            EncounterCommand cmd;
            cmd.type = EncounterCommand::Type::Spawn;
            cmd.statId = id;
            return cmd;
        };
        PushButton(buttons, left, y, third, kBtnH, goblin ? goblin->name : "Goblin",
                   spawnCmd("goblin"), bottom);
        PushButton(buttons, left + third + kGap, y, third, kBtnH,
                   skeleton ? skeleton->name : "Skeleton", spawnCmd("skeleton"), bottom);
        PushButton(buttons, left + (third + kGap) * 2.0f, y, third, kBtnH,
                   bugbear ? bugbear->name : "Bugbear", spawnCmd("bugbear"), bottom);
        y += kBtnH + kGap;

        const f32 half = (inner - kGap) * 0.5f;
        EncounterCommand party;
        party.type = EncounterCommand::Type::ToggleParty;
        PushButton(buttons, left, y, half, kBtnH,
                   m_builder->IncludeParty() ? "Party: On" : "Party: Off", party, bottom);
        EncounterCommand roll;
        roll.type = EncounterCommand::Type::Roll;
        PushButton(buttons, left + half + kGap, y, half, kBtnH, "Roll Init", roll, bottom);
        y += kBtnH + kGap;

        const u32 currentId = m_builder->CurrentTurn() ? m_builder->CurrentTurn()->id : 0;
        i32 shown = 0;
        for (const EncounterMember& member : m_builder->Members()) {
            if (shown >= 6) break;
            char caption[96];
            if (member.initiative >= 0) {
                std::snprintf(caption, sizeof(caption), "%s  %d/%d  AC %d  Init %d",
                              member.name.c_str(), member.currentHp, member.maxHp,
                              member.armorClass, member.initiative);
            } else {
                std::snprintf(caption, sizeof(caption), "%s  %d/%d  AC %d",
                              member.name.c_str(), member.currentHp, member.maxHp,
                              member.armorClass);
            }
            EncounterCommand select;
            select.type = EncounterCommand::Type::Select;
            select.memberId = member.id;
            const f32 next = PushButton(buttons, left, y, inner, kRowH, caption, select, bottom);
            if (!buttons.empty() && buttons.back().command.memberId == member.id &&
                buttons.back().command.type == EncounterCommand::Type::Select) {
                buttons.back().selected = (member.id == m_selected);
                buttons.back().current = m_builder->IsCombatActive() && member.id == currentId;
            }
            y = next + 1.0f;
            ++shown;
        }
        if (shown == 0) {
            y += 2.0f;
        }
        y += kGap;

        const f32 quarter = (inner - kGap * 3.0f) / 4.0f;
        auto amountCmd = [](EncounterCommand::Type type, i32 amount) {
            EncounterCommand cmd;
            cmd.type = type;
            cmd.amount = amount;
            return cmd;
        };
        PushButton(buttons, left, y, quarter, kBtnH, "Dmg 5",
                   amountCmd(EncounterCommand::Type::Damage, 5), bottom);
        PushButton(buttons, left + (quarter + kGap), y, quarter, kBtnH, "Heal 5",
                   amountCmd(EncounterCommand::Type::Heal, 5), bottom);
        PushButton(buttons, left + (quarter + kGap) * 2.0f, y, quarter, kBtnH, "Full HP",
                   amountCmd(EncounterCommand::Type::Full, 0), bottom);
        EncounterCommand remove;
        remove.type = EncounterCommand::Type::Remove;
        PushButton(buttons, left + (quarter + kGap) * 3.0f, y, quarter, kBtnH, "Remove", remove, bottom);
        y += kBtnH + kGap;

        EncounterCommand start;
        start.type = EncounterCommand::Type::Start;
        EncounterCommand next;
        next.type = EncounterCommand::Type::Next;
        EncounterCommand end;
        end.type = EncounterCommand::Type::End;
        PushButton(buttons, left, y, third, kBtnH,
                   m_builder->IsCombatActive() ? "Combat On" : "Start", start, bottom);
        PushButton(buttons, left + third + kGap, y, third, kBtnH, "Next Turn", next, bottom);
        PushButton(buttons, left + (third + kGap) * 2.0f, y, third, kBtnH, "End", end, bottom);
        y += kBtnH + kGap + 2.0f;
        return y;
    }

    EncounterBuilder* m_builder = nullptr;
    std::vector<EncounterCommand> m_commands;
    u32 m_selected = 0;
    b8 m_wasDown = false;
};

EncounterPanel::EncounterPanel() {
    m_view = new View(&m_builder);
    m_dockPanel = new DockPanel("Encounter", "Encounter");
    m_dockPanel->SetContent(m_view);
    m_dockPanel->SetDocked(false);
    // Sits in the viewport gap between the top and bottom docks at 1280x720.
    // Engine::Initialize nudges this to the real window size.
    m_dockPanel->SetPosition(Vec2(262.0f, 208.0f));
    m_dockPanel->SetSize(Vec2(360.0f, 300.0f));
    m_dockPanel->SetFloatingPos(Vec2(262.0f, 208.0f));
    m_dockPanel->SetCloseCallback([this]() {
        if (m_dockPanel) m_dockPanel->SetVisible(false);
    });
}

EncounterPanel::~EncounterPanel() {
    for (const EncounterMember& member : m_builder.Members()) {
        m_bridge.Despawn(member.entity);
    }
    m_bridge.Shutdown();
}

void EncounterPanel::SetOpen(b8 open) {
    if (!m_dockPanel) return;
    m_dockPanel->SetVisible(open);
}

b8 EncounterPanel::IsOpen() const {
    return m_dockPanel && m_dockPanel->IsVisible();
}

void EncounterPanel::Update(Scene* scene) {
    m_bridge.Bind(scene);
    if (!m_view || !m_dockPanel || !m_dockPanel->IsVisible()) return;

    const std::vector<EncounterCommand> commands = m_view->TakeCommands();
    for (const EncounterCommand& cmd : commands) {
        switch (cmd.type) {
        case EncounterCommand::Type::Spawn: {
            const u32 id = m_builder.Add(cmd.statId);
            EncounterMember* member = m_builder.Find(id);
            if (!member) break;
            if (!m_bridge.Realize(*member)) {
                m_builder.Remove(id);
                DND_LOG_WARN("Encounter: could not spawn '%s' (no active scene)",
                             cmd.statId ? cmd.statId : "?");
                break;
            }
            m_selected = id;
            EditorSettings::Get().selectedEntity = member->entity;
            break;
        }
        case EncounterCommand::Type::Select: {
            m_selected = cmd.memberId;
            if (const EncounterMember* member = m_builder.Find(m_selected)) {
                if (member->entity.IsValid()) {
                    EditorSettings::Get().selectedEntity = member->entity;
                }
            }
            break;
        }
        case EncounterCommand::Type::ToggleParty:
            m_builder.SetIncludeParty(!m_builder.IncludeParty());
            DND_LOG_INFO("Encounter: party initiative %s",
                         m_builder.IncludeParty() ? "on" : "off");
            break;
        case EncounterCommand::Type::Roll: {
            std::vector<InitiativeInput> extras;
            if (m_builder.IncludeParty()) extras = m_bridge.CollectParty(m_builder);
            else m_bridge.ClearParty();
            const b8 rolled = m_builder.RollInitiative([]() { return DiceRoller::Roll(1, 20); }, extras);
            m_bridge.SyncInitiative(m_builder);
            // Push even when the roll comes up empty so an in-progress combat
            // is cleared instead of keeping the previous turn order.
            m_bridge.PushCombatState(m_builder);
            if (!rolled) {
                DND_LOG_INFO("Encounter: nobody alive to roll initiative");
                break;
            }
            DND_LOG_INFO("Encounter initiative (%zu):", m_builder.Order().size());
            for (const InitiativeRoll& roll : m_builder.Order()) {
                DND_LOG_INFO("  %s total %d (d20 %d)", roll.label.c_str(), roll.total, roll.d20);
            }
            break;
        }
        case EncounterCommand::Type::Damage:
            if (m_builder.ApplyDamage(m_selected, cmd.amount)) {
                if (const EncounterMember* member = m_builder.Find(m_selected)) {
                    DND_LOG_INFO("Encounter: %s takes %d -> HP %d/%d",
                                 member->name.c_str(), cmd.amount,
                                 member->currentHp, member->maxHp);
                }
                m_bridge.PushCombatState(m_builder);
            }
            break;
        case EncounterCommand::Type::Heal:
            if (m_builder.Heal(m_selected, cmd.amount)) {
                if (const EncounterMember* member = m_builder.Find(m_selected)) {
                    DND_LOG_INFO("Encounter: %s heals %d -> HP %d/%d",
                                 member->name.c_str(), cmd.amount,
                                 member->currentHp, member->maxHp);
                }
            }
            break;
        case EncounterCommand::Type::Full:
            if (const EncounterMember* member = m_builder.Find(m_selected)) {
                m_builder.SetCurrentHp(m_selected, member->maxHp);
                DND_LOG_INFO("Encounter: %s set to full HP %d",
                             member->name.c_str(), member->maxHp);
                m_bridge.PushCombatState(m_builder);
            }
            break;
        case EncounterCommand::Type::Start:
            if (m_builder.IsCombatActive()) break;
            if (m_builder.StartCombat()) {
                m_bridge.PushCombatState(m_builder);
                if (const InitiativeRoll* turn = m_builder.CurrentTurn()) {
                    DND_LOG_INFO("Encounter combat started. Turn: %s (%d)",
                                 turn->label.c_str(), turn->total);
                }
            } else {
                DND_LOG_INFO("Encounter: roll initiative before starting combat");
            }
            break;
        case EncounterCommand::Type::Next:
            if (m_builder.NextTurn()) {
                m_bridge.PushCombatState(m_builder);
                if (const InitiativeRoll* turn = m_builder.CurrentTurn()) {
                    DND_LOG_INFO("Encounter next turn: %s (%d)", turn->label.c_str(), turn->total);
                }
            }
            break;
        case EncounterCommand::Type::End:
            m_builder.EndCombat();
            m_bridge.PushCombatState(m_builder);
            DND_LOG_INFO("Encounter combat ended");
            break;
        case EncounterCommand::Type::Remove: {
            const EncounterMember* member = m_builder.Find(m_selected);
            if (!member) break;
            const Entity entity = member->entity;
            const std::string name = member->name;
            m_bridge.Despawn(entity);
            m_builder.Remove(m_selected);
            m_bridge.PushCombatState(m_builder);
            m_selected = 0;
            DND_LOG_INFO("Encounter: removed %s", name.c_str());
            break;
        }
        }
    }

    m_view->SetSelected(m_selected);
    const u32 turnId = m_builder.CurrentTurn() ? m_builder.CurrentTurn()->id : 0;
    for (const EncounterMember& member : m_builder.Members()) {
        const b8 current = m_builder.IsCombatActive() && member.id == turnId;
        m_bridge.SyncVisual(member, current);
    }
}

} // namespace Dnd
