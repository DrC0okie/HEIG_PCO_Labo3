#include "factory.h"

#include <pcosynchro/pcothread.h>

#include <iostream>
#include <mutex>

#include "costs.h"
#include "extractor.h"
#include "wholesale.h"

WindowInterface* Factory::interface = nullptr;


Factory::Factory(int uniqueId, int fund, ItemType builtItem,
                 std::vector<ItemType> resourcesNeeded)
    : Seller(fund, uniqueId),
      resourcesNeeded(resourcesNeeded),
      itemBuilt(builtItem),
      nbBuild(0) {
    assert(builtItem == ItemType::Chip || builtItem == ItemType::Plastic ||
           builtItem == ItemType::Robot);

    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Factory created");
}

void Factory::setWholesalers(std::vector<Wholesale*> wholesalers) {
    Factory::wholesalers = wholesalers;

    for (Seller* seller : wholesalers) {
        interface->setLink(uniqueId, seller->getUniqueId());
    }
}

ItemType Factory::getItemBuilt() {
    return itemBuilt;
}

int Factory::getMaterialCost() {
    return getCostPerUnit(itemBuilt);
}

bool Factory::verifyResources() {
    for (auto item : resourcesNeeded) {
        if (stocks[item] == 0) {
            return false;
        }
    }

    return true;
}

void Factory::buildItem() {
    int salary = getEmployeeSalary(getEmployeeThatProduces(itemBuilt));

    if (money < salary) {
        return;
    }
    // If we have enough money to pay salary

    runMutex.lock();

    // produce item
    for (ItemType item : resourcesNeeded) {
        stocks[item]--;
    }

    // Pay salary
    money -= salary;
    runMutex.unlock();

    // Temps simulant l'assemblage d'un objet.
    PcoThread::usleep((rand() % 100) * 100000);

    runMutex.lock();
    // Increment number of payed employee
    nbBuild++;

    // update item stock
    stocks[itemBuilt]++;
    runMutex.unlock();

    // Update interface
    interface->consoleAppendText(uniqueId, "Factory have build a new object");
}

void Factory::orderResources() {
    orderMutex.lock();
    auto res = resourcesNeeded;

    // Sort based on stocks, prioritizing resources the factory has the least of
    std::sort(res.begin(), res.end(),
              [this](const ItemType& i1, const ItemType& i2) -> bool {
                  return stocks[i1] < stocks[i2];
              });

    // Iterate over the sorted resources
    for (auto it : res) {
        // Check if we have enough money
        int cost = getCostPerUnit(it);

        if (money < cost) {
            // We can't buy this resource => stop trying to purchase others
            // since they are less critical
            break;
        }

        // Iterate over available wholesalers
        for (Wholesale* ws : wholesalers) {
            // If the trade is successful, update stocks and money
            if (ws->trade(it, 1) == cost) {
                stocks[it]++;
                money -= cost;
            }
        }
    }
    orderMutex.unlock();

    // Temps de pause pour éviter trop de demande
    PcoThread::usleep(10 * 100000);
}

void Factory::run() {
    if (wholesalers.empty()) {
        std::cerr << "You have to give to factories wholesalers to sales their "
                     "resources"
                  << std::endl;
        return;
    }
    interface->consoleAppendText(uniqueId, "[START] Factory routine");

    while (!PcoThread::thisThread()->stopRequested()) {
        if (verifyResources()) {
            buildItem();
        } else {
            orderResources();
        }
        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
    }
    interface->consoleAppendText(uniqueId, "[STOP] Factory routine");
}

std::map<ItemType, int> Factory::getItemsForSale() {
    return std::map<ItemType, int>({
        {itemBuilt, stocks[itemBuilt]}
    });
}

int Factory::trade(ItemType it, int qty) {
    if (qty <= 0 || it != itemBuilt || stocks[itemBuilt] <= 0)
        return 0;

    int cost = getCostPerUnit(it) * qty;

    tradeMutex.lock();
    money += cost;
    stocks[it] -= qty;
    tradeMutex.unlock();

    return cost;
}

int Factory::getAmountPaidToWorkers() {
    return Factory::nbBuild *
           getEmployeeSalary(getEmployeeThatProduces(itemBuilt));
}

void Factory::setInterface(WindowInterface* windowInterface) {
    interface = windowInterface;
}

PlasticFactory::PlasticFactory(int uniqueId, int fund)
    : Factory::Factory(uniqueId, fund, ItemType::Plastic, {ItemType::Petrol}) {}

ChipFactory::ChipFactory(int uniqueId, int fund)
    : Factory::Factory(uniqueId, fund, ItemType::Chip,
                       {ItemType::Sand, ItemType::Copper}) {}

RobotFactory::RobotFactory(int uniqueId, int fund)
    : Factory::Factory(uniqueId, fund, ItemType::Robot,
                       {ItemType::Chip, ItemType::Plastic}) {}
