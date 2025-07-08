#ifndef ADVANCED_ITEM_REGISTRY_H
#define ADVANCED_ITEM_REGISTRY_H

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

#include "advanced_line_item.h"
#include "spdlog/spdlog.h"

class QWidget;
class QGraphicsItem;

class AdvancedItemRegistry {

public:
    template <typename T>
    struct ItemEntry {
        T* graphics_item;
        typename T::ControlPanel* widget;
        typename T::State* state;
    };

    template <typename T>
    void add_item(T* item)
    {
        typename T::State* state = &item->state();
        typename T::ControlPanel* control_panel = new typename T::ControlPanel(*state);
        map.emplace(item->key(), ItemEntry<T>{ item, control_panel, state });
    }

    template <typename ItemType>
    std::optional<ItemEntry<ItemType>> get_item_entry(std::string key)
    {
        if (map.find(key) == std::end(map)) {
            spdlog::critical("Inexistent requested key: {0}", key);
            return std::nullopt;
        }

        auto v = map.at(key);
        using T = std::decay_t<decltype(v)>;
        if (!std::holds_alternative<ItemEntry<ItemType>>(v)) {
            spdlog::critical("Requested item with key: {0} does not match requested data type", key);
            return std::nullopt;
        }

        return std::get<ItemEntry<ItemType>>(v);
    }

private:
    using MapEntryType = std::variant<ItemEntry<AdvancedLineItem>>;
    std::unordered_map<std::string, MapEntryType> map;
};

#endif /* ADVANCED_ITEM_REGISTRY_H */
