[Click here](../README.md) to view the README.

## Design and implementation

The design of this application is minimalistic to get started with code examples on PSOC&trade; Edge MCU devices. All PSOC&trade; Edge E84 MCU applications have a dual-CPU three-project structure to develop code for the CM33 and CM55 cores. The CM33 core has two separate projects for the secure processing environment (SPE) and non-secure processing environment (NSPE). A project folder consists of various subfolders, each denoting a specific aspect of the project. The three project folders are as follows:

**Table 1. Application projects**

Project | Description
--------|------------------------
*proj_cm33_s* | Project for CM33 secure processing environment (SPE)
*proj_cm33_ns* | Project for CM33 non-secure processing environment (NSPE)
*proj_cm55* | CM55 project

<br>

In this code example, at device reset, the secure boot process starts from the ROM boot with the secure enclave (SE) as the root of trust (RoT). From the secure enclave, the boot flow is passed on to the system CPU subsystem where the secure CM33 application starts. After all necessary secure configurations, the flow is passed on to the non-secure CM33 application. Resource initialization for this example is performed by this CM33 non-secure project. It configures the system clocks, pins, clock to peripheral connections, and other platform resources. It then enables the CM55 core using the `Cy_SysEnableCM55()` function and the CM55 core is subsequently put to DeepSleep mode.


In the CM33 non-secure application, the clocks and system resources are initialized by the BSP initialization function. This code example features one low-power comparator (LPComp) peripheral, User LED1, one GPIO for the wakeup input, and one potentiometer on the Vplus pin.

**Figure 1** shows the firmware flow. The main loop checks the output of LPComp Channel 0 and toggles the LED1 when the output is HIGH. Otherwise, the system goes into the Hibernate mode after turning the LED1 ON for two seconds. The system will wake up immediately if the LPComp Channel 0 output goes HIGH during Hibernate mode as shown in **Figure 1**.

**Figure 1. Firmware flow**

![](../images/flow-diagram.png)
