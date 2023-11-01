#include "factory.h"
#include "extractor.h"
#include "costs.h"
#include "wholesale.h"
#include <pcosynchro/pcothread.h>
#include <iostream>
#include <mutex>

WindowInterface* Factory::interface = nullptr;


Factory::Factory(int uniqueId, int fund, ItemType builtItem, std::vector<ItemType> resourcesNeeded)
    : Seller(fund, uniqueId), resourcesNeeded(resourcesNeeded), itemBuilt(builtItem), nbBuild(0)
{
    assert(builtItem == ItemType::Chip ||
           builtItem == ItemType::Plastic ||
           builtItem == ItemType::Robot);

    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Factory created");
}

void Factory::setWholesalers(std::vector<Wholesale *> wholesalers) {
    Factory::wholesalers = wholesalers;

    for(Seller* seller: wholesalers){
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
    static PcoMutex buildMutex = PcoMutex();

    // If we have enough money to pay salary
    if(money >= salary){
        buildMutex.lock();

        // produce item
        for(ItemType item : resourcesNeeded) {
            stocks[item]--;
        }

        //Pay salary
        money -= salary;
        buildMutex.unlock();
    }else{
        return;
    }

    //Temps simulant l'assemblage d'un objet.
#ifdef NO_SLEEP
    PcoThread::usleep((rand() % 100) * 100000);
#endif

    buildMutex.lock();
    // Increment number of payed employee
    nbBuild++;

    // update item stock
    stocks[itemBuilt]++;
    buildMutex.unlock();

    // Update interface
    interface->consoleAppendText(uniqueId, "Factory have build a new object");
}

void Factory::orderResources() {

    // Using lock_guard for cleaner management of lock
    static PcoMutex orderMutex = PcoMutex();
    std::lock_guard<PcoMutex> guard(orderMutex);

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

        if (money < cost){
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

    //Temps de pause pour éviter trop de demande
#ifdef NO_SLEEP
    PcoThread::usleep(10 * 100000);
#endif
}

void Factory::run() {
    if (wholesalers.empty()) {
        std::cerr << "You have to give to factories wholesalers to sales their resources" << std::endl;
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
    return std::map<ItemType, int>({{itemBuilt, stocks[itemBuilt]}});
}

int Factory::trade(ItemType it, int qty) {

    static PcoMutex tradeMutex = PcoMutex();

    if(qty <= 0)
        return 0;

    // Here we use a lock_guard to ensure the lock is always released
    // even in case of an exception
    std::lock_guard<PcoMutex> guard(tradeMutex);

    auto availableItems = getItemsForSale();
    auto item = availableItems.find(it);

    // check if the asked item is available in sufficient quantity
    if(item != availableItems.end() && qty <= item->second){

        int cost = getCostPerUnit(it) * qty;

        // Transaction
        money += cost;
        stocks[it] -= qty;

        return cost;
    }

    return 0;
}

int Factory::getAmountPaidToWorkers() {
    return Factory::nbBuild * getEmployeeSalary(getEmployeeThatProduces(itemBuilt));
}

void Factory::setInterface(WindowInterface *windowInterface) {
    interface = windowInterface;
}

PlasticFactory::PlasticFactory(int uniqueId, int fund) :
    Factory::Factory(uniqueId, fund, ItemType::Plastic, {ItemType::Petrol}) {}

ChipFactory::ChipFactory(int uniqueId, int fund) :
    Factory::Factory(uniqueId, fund, ItemType::Chip, {ItemType::Sand, ItemType::Copper}) {}

RobotFactory::RobotFactory(int uniqueId, int fund) :
    Factory::Factory(uniqueId, fund, ItemType::Robot, {ItemType::Chip, ItemType::Plastic}) {}
