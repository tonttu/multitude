include(../multitude.pri)

CONFIG -= qt

win32:CONFIG += staticlib

S1 += Collision/b2BroadPhase.cpp \
  Collision/b2CollideCircle.cpp \
  Collision/b2CollidePolygon.cpp \
  Collision/b2Collision.cpp \
  Collision/b2Distance.cpp \
  Collision/b2DynamicTree.cpp \
  Collision/b2TimeOfImpact.cpp

S2 +=  Collision/Shapes/b2CircleShape.cpp \
  Collision/Shapes/b2PolygonShape.cpp

S3 += Common/b2BlockAllocator.cpp \
  Common/b2Math.cpp \
  Common/b2Settings.cpp \
  Common/b2StackAllocator.cpp

S4 += Dynamics/b2Body.cpp \
  Dynamics/b2ContactManager.cpp \
  Dynamics/b2Fixture.cpp \
  Dynamics/b2Island.cpp \
  Dynamics/b2World.cpp \
  Dynamics/b2WorldCallbacks.cpp

S5 += Dynamics/Contacts/b2CircleContact.cpp \
  Dynamics/Contacts/b2Contact.cpp \
  Dynamics/Contacts/b2ContactSolver.cpp \
  Dynamics/Contacts/b2PolygonAndCircleContact.cpp \
  Dynamics/Contacts/b2PolygonContact.cpp \
  Dynamics/Contacts/b2TOISolver.cpp

S6 += Dynamics/Joints/b2DistanceJoint.cpp \
  Dynamics/Joints/b2FrictionJoint.cpp \
  Dynamics/Joints/b2GearJoint.cpp \
  Dynamics/Joints/b2Joint.cpp \
  Dynamics/Joints/b2LineJoint.cpp \
  Dynamics/Joints/b2MouseJoint.cpp \
  Dynamics/Joints/b2PrismaticJoint.cpp \
  Dynamics/Joints/b2PulleyJoint.cpp \
  Dynamics/Joints/b2RevoluteJoint.cpp \
  Dynamics/Joints/b2WeldJoint.cpp

H1 += Collision/b2BroadPhase.h \
  Collision/b2Collision.h \
  Collision/b2Distance.h \
  Collision/b2DynamicTree.h \
  Collision/b2TimeOfImpact.h

H2 += Collision/Shapes/b2CircleShape.h \
  Collision/Shapes/b2PolygonShape.h \
  Collision/Shapes/b2Shape.h

H3 += Common/b2BlockAllocator.h \
  Common/b2Math.h \
  Common/b2Settings.h \
  Common/b2StackAllocator.h

H4 += Dynamics/b2Body.h \
  Dynamics/b2ContactManager.h \
  Dynamics/b2Fixture.h \
  Dynamics/b2Island.h \
  Dynamics/b2TimeStep.h \
  Dynamics/b2World.h \
  Dynamics/b2WorldCallbacks.h

H5 += Dynamics/Contacts/b2CircleContact.h \
  Dynamics/Contacts/b2Contact.h \
  Dynamics/Contacts/b2ContactSolver.h \
  Dynamics/Contacts/b2PolygonAndCircleContact.h \
  Dynamics/Contacts/b2PolygonContact.h \
  Dynamics/Contacts/b2TOISolver.h

H6 += Dynamics/Joints/b2DistanceJoint.h \
  Dynamics/Joints/b2FrictionJoint.h \
  Dynamics/Joints/b2GearJoint.h \
  Dynamics/Joints/b2Joint.h \
  Dynamics/Joints/b2LineJoint.h \
  Dynamics/Joints/b2MouseJoint.h \
  Dynamics/Joints/b2PrismaticJoint.h \
  Dynamics/Joints/b2PulleyJoint.h \
  Dynamics/Joints/b2RevoluteJoint.h \
  Dynamics/Joints/b2WeldJoint.h 

h1.path = /include/$$TARGET/Collision
h1.files = $$H1

h2.path = /include/$$TARGET/Collision/Shapes
h2.files = $$H2

h3.path = /include/$$TARGET/Common
h3.files = $$H3

h4.path = /include/$$TARGET/Dynamics
h4.files = $$H4

h5.path = /include/$$TARGET/Dynamics/Contacts
h5.files = $$H5

h6.path = /include/$$TARGET/Dynamics/Joints
h6.files = $$H6

s1.path = /src/MultiTouch/multitude/$$TARGET/Collision
s1.files = $$S1 $$H1

s2.path = /src/MultiTouch/multitude/$$TARGET/Collision/Shapes
s2.files = $$S2 $$H2

s3.path = /src/MultiTouch/multitude/$$TARGET/Common
s3.files = $$S3 $$H3

s4.path = /src/MultiTouch/multitude/$$TARGET/Dynamics
s4.files = $$S4 $$H4

s5.path = /src/MultiTouch/multitude/$$TARGET/Dynamics/Contacts
s5.files = $$S5 $$H5

s6.path = /src/MultiTouch/multitude/$$TARGET/Dynamics/Joints
s6.files = $$S6 $$H6

unix:QMAKE_CXXFLAGS_RELEASE += -ffast-math

INSTALLS += s1 s2 s3 s4 s5 s6 h1 h2 h3 h4 h5 h6

EXPORT_HEADERS = Box2D.h
EXPORT_SOURCES = Box2D.h

SOURCES += $$S1 $$S2 $$S3 $$S4 $$S5 $$S6
HEADERS += $$H1 $$H2 $$H3 $$H4 $$H5 $$H6

include(../library.pri)
