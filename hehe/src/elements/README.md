# Elements

---

## Overview
Each of these file in this directory will have

1. A data struct
2. A property struct

The data struct will be public defined in the .h file, while the property struct will be anonymous in the .c file.

The data struct must contain:

- The data about a particular element that is defined in the moment and changes
- An ID into the property struct telling which property that particular element will have.
- This will have the size telling how many elements are/can be there.

The property struct contains:

- Runtime constant properties of each type of element that can exist.
- This will have the size telling how many types of elements there are.

API shall be provided by each file allowing access into the anonymous properties which will be used to do logic.

**Failure in loading data struct can be considered a soft error, and can be ignored by**
**things like not rendering and not applying logic to corrupted data.**
**Failure in loading property struct will be considered a critical error, and may even**
**warrant crashing.**

---

---

## How loading will work

Entire data of per map will be loaded when entering the map and then unloaded/repurposed during the loading of new map.

---
