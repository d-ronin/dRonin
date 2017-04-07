#ifndef UAVOBJECTSTEST_H
#define UAVOBJECTSTEST_H

#include "..\exampleobject1.h"
#include "..\uavobjectmanager.h"
#include <QTextStream>
#include <QTimer>

class UAVObjectsTest : QObject
{
    Q_OBJECT

public:
    UAVObjectsTest();

private slots:
    void objectUpdated(UAVObject *obj);
    void objectUpdatedAuto(UAVObject *obj);
    void objectUpdatedManual(UAVObject *obj);
    void objectUpdatedPeriodic(UAVObject *obj);
    void objectUnpacked(UAVObject *obj);
    void updateRequested(UAVObject *obj);
    void runTest();
    void newObject(UAVObject *obj);
    void newInstance(UAVObject *obj);

private:
    UAVObjectManager *objMngr;
    ExampleObject1 *obj1;
    QTimer *timer;
    QTextStream sout;
    bool done;
};

#endif // UAVOBJECTSTEST_H
