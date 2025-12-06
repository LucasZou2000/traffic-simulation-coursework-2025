#include "../includes/TaskBuilder.hpp"

int TaskBuilder::buildItemRequirement(int item_id, int quantity, TaskGraph& g) {
    // 查询是否有合成配方
    const CraftingRecipe* recipe = nullptr;
    const std::map<int, CraftingRecipe>& all = crafting_.getAllRecipes();
    for (std::map<int, CraftingRecipe>::const_iterator it = all.begin(); it != all.end(); ++it) {
        if (it->second.product_item_id == item_id) {
            recipe = &(it->second);
            break;
        }
    }

    TGNode node;
    node.item_id = item_id;
    node.required = quantity;

    if (recipe) {
        node.type = TaskType::Craft;
        node.crafting_id = recipe->crafting_id;
        int produced = recipe->quantity_produced > 0 ? recipe->quantity_produced : 1;
        int batches = (quantity + produced - 1) / produced;
        int parent = g.addNode(node);
        for (size_t i = 0; i < recipe->materials.size(); ++i) {
            int mat_qty = recipe->materials[i].quantity_required * batches;
            int child = buildItemRequirement(recipe->materials[i].item_id, mat_qty, g);
            g.addEdge(parent, child);
        }
        return parent;
    } else {
        node.type = TaskType::Gather;
        return g.addNode(node);
    }
}

int TaskBuilder::buildForBuilding(const Building& building, TaskGraph& g) {
    TGNode build;
    build.type = TaskType::Build;
    build.item_id = 10000 + building.building_id;
    build.building_id = building.building_id;
    build.required = 1;
    int root = g.addNode(build);
    for (size_t i = 0; i < building.required_materials.size(); ++i) {
        int mat_id = building.required_materials[i].first;
        int mat_qty = building.required_materials[i].second;
        int child = buildItemRequirement(mat_id, mat_qty, g);
        g.addEdge(root, child);
    }
    return root;
}
