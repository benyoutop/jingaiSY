# Design and Implementation of a Multidimensional Smart Manhole Monitoring System for Smart Cities

## Abstract
With the continuous advancement of smart city development, intelligent sensing and safety management of urban infrastructure have become important research topics. As an essential component of underground utility networks, manhole covers are numerous, widely distributed, and exposed to complex operating environments. Once abnormal conditions occur, they may lead to traffic-safety risks and municipal operation hazards. To address the low efficiency and poor real-time performance of manual inspection, as well as the limited sensing dimensions of existing solutions, this paper designs and implements a multidimensional smart manhole monitoring system for smart city applications. Based on an embedded terminal, the system establishes an integrated monitoring chain that combines state acquisition, edge-side judgment, local warning, and remote data upload, and enables coordinated sensing of underground water-level variation, attitude disturbance, smoke anomalies, and location information. In terms of methodology, a lightweight edge monitoring mechanism combining periodic sampling and threshold-based decision rules is adopted, and multi-source data are uploaded through a telemetry interface for platform-side coordination. This paper presents the overall system architecture, key implementation methods, and multi-source fusion criteria, and shows the operating results of the system prototype and the frontend monitoring interfaces. The results show that the proposed system features a clear structure, convenient deployment, and high integration, and can provide a useful reference for the real-time monitoring and maintenance of smart manhole facilities in urban environments.

## Keywords
smart city; manhole monitoring; multi-sensor fusion; Internet of Things; embedded system

## 1 Introduction
With the continuous advancement of smart city development, the digital, networked, and intelligent management of urban infrastructure has become an important direction in both research and practical application [21]. As a key component of urban underground utility networks, manhole covers are widely distributed along roads, in residential communities, industrial parks, and around various municipal facilities, serving as critical access nodes for underground drainage, power, communication, and gas pipeline systems. The operating condition of manhole covers is directly related to urban traffic safety, public environmental safety, and the stable operation of underground facilities. Once abnormal situations such as loss, unauthorized opening, displacement and tilting, looseness, or smoke accumulation in underground shafts occur, they may not only affect the normal operation of municipal facilities but also cause safety incidents such as vehicle damage, pedestrian falls, fires or explosions, and hazardous gas leakage [14,18-20]. Therefore, the development of an intelligent sensing and warning system for manhole scenarios is of substantial engineering value and practical significance.

Traditional manhole management mainly relies on manual inspection and passive repair. Although this approach is simple to implement, it generally suffers from long inspection cycles, limited coverage, untimely fault discovery, and high maintenance costs, making it difficult to satisfy the requirements of refined urban governance for real-time sensing, rapid localization, and proactive warning [18,20]. With the development of IoT, embedded systems, wireless communication, and cloud-platform technologies, an increasing number of studies have applied sensors, microcontrollers, and remote communication technologies to manhole monitoring, enabling the real-time acquisition and transmission of manhole state information to remote platforms for monitoring, alarm generation, and maintenance coordination [8-12,14,16-18,20-23]. Compared with traditional manual inspection, smart manhole monitoring systems exhibit clear advantages in sensing efficiency, response speed, and information-based management.

From the perspective of domestic and international research progress, smart manhole systems have gradually evolved from single-state detection to multi-source sensing and platform coordination. Overseas studies have paid more attention to ecological design, low-power anomaly recognition, and multi-event fusion alarms. For example, some studies have established ecological design index systems for intelligent manhole covers based on fuzzy analytic hierarchy processes [1], while others have proposed low-power anomaly-recognition methods based on differences in vibration patterns in order to balance real-time response and device energy consumption [2]. In addition, multi-event fusion alarm methods have effectively reduced the false alarm rate of single-sensor solutions by jointly analyzing multi-sensor state signals [3]. In broader smart city and IoT research, review studies have summarized development trends in city-scale monitoring systems from the perspectives of infrastructure sensing, IoT architecture, and LPWAN evolution [21-23]. Engineering-oriented studies on manhole scenarios focus more closely on practical deployment needs and involve multiple communication technologies, such as NB-IoT, LoRa, and WiFi, as well as combinations of sensors for tilt, water level, gas, temperature and humidity, ultrasonic ranging, and positioning [4-10,14,16,18-20]. Some studies emphasize low-power remote monitoring terminals based on NB-IoT and similar wide-area communication methods [5-6,14,16,20], while others introduce cloud platforms, remote warning, and intelligent analysis to improve platform-side visualization and data-management capability [4,9-10,13]. Additional work has extended smart manhole systems by incorporating joint sensing of water level and hazardous gases, security coordination, and context awareness [7,15-16,18,20]. Overall, existing studies provide abundant references for the development of intelligent manhole technologies and indicate that manhole monitoring is gradually evolving from “single-point alarms” to “multidimensional sensing, remote coordination, and intelligent maintenance.”

Despite this progress, several common limitations remain. First, many systems are designed around a single risk dimension, such as tilt detection, underground gas monitoring, or open-cover detection, and therefore cannot simultaneously cover underground water-level variation, attitude disturbance, smoke anomalies, and positioning information [14,16,18]. Second, although some solutions possess strong cloud-side processing capability, their edge-side anomaly judgment and local coordination capabilities remain weak, resulting in insufficient support for short-term anomalies and rapid on-site response [3,9,24]. Third, some studies employ wide-area communication technologies such as NB-IoT and LoRa, which are more suitable for large-scale deployment, whereas short-range wireless access schemes still offer lower development thresholds and more flexible deployment in local networking, system debugging, and low-cost implementation [8,22-23]. Therefore, it is necessary to design a smart manhole monitoring terminal with a clear architecture, moderate implementation cost, and multidimensional sensing capability according to practical application needs.

Against this background, this paper designs and implements a multidimensional smart manhole monitoring system for smart city scenarios. The system is built around the STM32F103 microcontroller and integrates an ultrasonic ranging module, an MPU6050 attitude and motion sensor, an MQ-2 smoke sensor, a GPS module, an ESP8266 wireless communication module, and a local audio-visual alarm unit. The resulting terminal combines state acquisition, edge-side decision-making, local warning, and remote telemetry upload. In this system, the ultrasonic module is used to detect underground water-level variation; the MPU6050 is used to identify manhole tilt and motion disturbance; the MQ-2 is used to detect smoke anomalies; the GPS module provides positioning and time information; and the ESP8266 enables WiFi access and HTTP telemetry upload. LEDs and the buzzer jointly provide on-site warning. Through multi-source information acquisition and lightweight threshold-based decision rules, the system realizes comprehensive monitoring of typical manhole anomaly states.

The main contributions of this paper can be summarized as follows. First, a multidimensional sensing chain for manhole scenarios is constructed by integrating water level, attitude, motion, smoke, and positioning information within a low-cost terminal, thereby realizing joint perception across multiple risk dimensions. Second, a lightweight edge monitoring workflow based on STM32F103 is designed to support local state judgment, audio-visual warning coordination, and wireless telemetry upload without introducing a complex operating system or high-computing-power platform. Third, a complete implementation path is provided, covering hardware architecture, software workflow, communication interfaces, and experimental validation, which can serve as a reference for the engineering application of smart manhole monitoring systems. The remainder of this paper is organized as follows. Section 2 introduces the overall system design. Section 3 presents the key technologies and the multi-source sensing mechanism. Section 4 gives the experiments and result analysis. Section 5 concludes the paper.

## 2 Overall System Design

### 2.1 Design Objectives and Architectural Concept
Manhole monitoring scenarios are characterized by dispersed deployment, complex operating environments, diverse anomaly types, and strict requirements on maintenance response time. Therefore, the overall system architecture must simultaneously satisfy the needs for multidimensional sensing, rapid edge-side response, remote network transmission, and centralized platform management. To address these requirements, this paper follows the design concept of “coordination between terminal sensing and edge-side judgment, parallel local warning and remote monitoring, and a balance between lightweight implementation and extensible services,” and constructs a layered smart manhole monitoring architecture for smart city applications.

From the perspective of functional organization, the overall architecture can be divided into five layers: the perception layer, the edge control layer, the communication transmission layer, the backend service layer, and the frontend presentation layer. The perception layer is responsible for acquiring raw state data from the manhole and its surrounding environment. The edge control layer performs sampling scheduling, data preprocessing, preliminary anomaly judgment, and local audio-visual coordination. The communication transmission layer delivers terminal telemetry data to remote interfaces. The backend service layer handles data reception, storage, real-time push, and interface organization. The frontend presentation layer provides device-status visualization, historical-record queries, and anomaly display. These layers form a complete closed loop in the order of “state acquisition, edge processing, wireless transmission, backend service, and frontend presentation,” enabling both local autonomy and remote centralized supervision.

Compared with solutions that rely solely on a single sensor or emphasize only cloud-side processing, this layered structure is more suitable for engineering deployment in manhole monitoring scenarios. On the one hand, multi-sensor coordination enhances the comprehensive representation of heterogeneous information such as opening state, tilt, vibration, smoke, and positioning. On the other hand, the edge side undertakes basic decision-making and local warning functions, allowing core monitoring capability to be maintained when communication is temporarily abnormal, while the platform side supports historical traceability, state visualization, and maintenance decision-making. Therefore, the proposed architecture emphasizes both terminal-side real-time performance and platform-side manageability and extensibility.

### 2.2 Layered Architecture Design
As shown in Fig. 1, the layered architecture of the system consists of an ultrasonic ranging module, an MPU6050 attitude and motion sensor, an MQ-2 smoke sensor, and a GPS module in the perception layer. These modules are used to obtain underground water-level variation, attitude tilt and dynamic disturbance, smoke anomalies, and geographic positioning information, respectively. These heterogeneous signals together form the basis of terminal-side state sensing, enabling the system to comprehensively describe manhole operating conditions from the three dimensions of water-environment state, structural state, and spatial state.

The edge control layer takes STM32F103 as the core controller and is responsible for multi-sensor interface management, periodic sampling scheduling, key state-quantity calculation, threshold judgment, and local alarm control. This layer is crucial for real-time response. On the one hand, the controller accesses heterogeneous data through peripherals such as GPIO, ADC, I2C, timer input capture, and UART. On the other hand, it converts raw sampled values into structured telemetry fields and executes lightweight local decision logic for anomalies such as tilt, motion, and smoke. Once an abnormal condition is detected, the system immediately drives LEDs and the buzzer to provide on-site warning, thereby shortening the anomaly feedback chain.

The communication transmission layer employs the ESP8266 wireless module to realize WiFi access and HTTP telemetry upload. Considering that the prototype system emphasizes low-cost implementation, development convenience, and rapid deployment in local networks, wide-area communication schemes such as NB-IoT and LoRa are not adopted. Instead, ESP8266 controlled by AT commands is selected as the wireless access unit. This layer acts as the data bridge between the terminal and the platform, and its main tasks include network connection establishment, TCP link maintenance, HTTP request transmission, and reconnection control under abnormal conditions.

The backend service layer is built using Node.js and Fastify, and is responsible for telemetry interface access, parameter validation, GPS-state compensation, JSONL-based historical data persistence, and SSE-based real-time event push. The frontend presentation layer is implemented as a Vue 3 web client for device overview, map-based display, historical-data queries, and anomaly-record visualization. By organizing real-time acquisition data, historical records, and abnormal events on the platform side, the system forms a complete service chain from terminal-side monitoring to backend processing and frontend presentation, thereby providing a good foundation for subsequent maintenance scheduling and functional extension.

From the perspective of information flow, raw data are first generated in the perception layer, processed in the edge control layer into unified telemetry messages, and then uploaded through the communication layer to the backend service layer. The backend performs storage, compensation, and distribution, and then pushes the results to the frontend monitoring interface. Meanwhile, the local alarm chain is directly driven by the edge control layer and does not depend on remote platform feedback. Thus, the system contains both an “instant terminal-side warning chain” and a “remote platform-side supervision chain,” and these two chains complement each other to support the monitoring task.

### 2.3 Functional Module Composition
To realize the layered architecture described above, the system organizes functional modules around four stages: sensing acquisition, edge processing, wireless transmission, and result feedback. STM32F103 serves as the terminal-side core control unit and is responsible for sampling scheduling, state processing, and communication control. The ultrasonic module, MPU6050, MQ-2, and GPS correspond to different sensing dimensions, namely water level, attitude and motion, smoke environment, and spatial position. ESP8266 is responsible for telemetry upload, while LEDs and the buzzer provide local anomaly indication. The responsibilities of these modules are clear, thus providing a structural basis for subsequent hardware-interface design and software-implementation analysis.

The main system components and their functions are listed in Table 1.

| Module | Interface | Main Function |
| --- | --- | --- |
| STM32F103 | Main controller | Sampling scheduling, edge-side decision-making, data packaging, and upload control |
| Ultrasonic ranging module | GPIO + TIM2 input capture | Detection of underground water-level variation |
| MPU6050 | I2C1 | Detection of tilt, vibration, and motion disturbance |
| MQ-2 | ADC1 + GPIO | Detection of smoke-concentration variation and alarm output |
| GPS module | USART1 | Provision of coordinates, satellite count, and UTC time |
| ESP8266 | USART3 | WiFi access and HTTP telemetry upload |
| LED indicator | GPIO | Local visual alarm indication |
| Buzzer | GPIO / driver interface | Local audible alarm indication |

Overall, the multi-source sensors constitute the information input side of the terminal, STM32F103 serves as the edge processing core, ESP8266 forms the wireless transmission channel, the audio-visual devices provide local feedback, and the backend and frontend together realize remote supervision and presentation. This module organization effectively supports multidimensional sensing and layered coordination in manhole monitoring scenarios.

### 2.4 Operating Mechanism and Working Mode
The software system adopts a lightweight main-loop operating mechanism. After power-on and the completion of peripheral initialization, the terminal enters a periodic operating state. On the one hand, it continuously acquires multi-source sensor data and completes basic preprocessing and preliminary anomaly judgment on the edge side. On the other hand, when upload conditions are met, it sends structured telemetry data to the remote platform through the wireless link. The platform side is then responsible for data reception, storage, push, and presentation, thereby forming a complete data closed loop from terminal monitoring to remote supervision.

From the perspective of working mode, the system adopts a cooperative mechanism characterized by “continuous local monitoring, immediate edge-side warning, and periodic remote upload.” This mode ensures rapid terminal-side response to abnormal events while also enabling the platform side to maintain a centralized grasp of historical states and overall operating conditions. In addition, when GPS is temporarily invalid or the WiFi link fluctuates, the system can still maintain local sampling and warning functions and resume upload when conditions recover, demonstrating a certain degree of degraded operation capability and engineering robustness.

## 3 Key Technologies and Multi-Source Sensing Mechanism

### 3.1 Multi-Source Sensing and Edge-Side Acquisition Mechanism
The terminal side of the proposed system adopts a multi-source sensing scheme to address the limitation that a single sensor cannot fully describe the operating condition of a manhole. Risks in underground manhole environments may be manifested as water-level variation, tilt, severe disturbance, smoke anomalies, or location changes. Therefore, the system must jointly characterize these conditions from the perspectives of water-environment state, structural state, and spatial state [3,15,18]. Based on this idea, the system integrates ultrasonic sensing, inertial sensing, smoke sensing, and GPS positioning within a single terminal, and STM32F103 performs unified acquisition and preliminary processing on the edge side.

For water-environment sensing, the ultrasonic module is used to characterize the relative distance from the sensor to the underground liquid surface. The system uses timer input capture to measure the echo pulse width and converts it into a distance value according to the approximate speed of sound. The basic conversion relationship is expressed as:

$$
\mathrm{distance\_mm}=\frac{\mathrm{echo\_us}\times 343}{2000}
$$

If the installation height of the sensor is known, the measured distance can be further converted into the current water-level height. Therefore, the ultrasonic channel mainly serves the purpose of monitoring underground water accumulation or liquid-level variation in urban manhole scenarios. To avoid sampling blockage caused by abnormal echoes, the system introduces a timeout protection mechanism so that the measurement process can remain continuous even under complex underground reflection conditions.

For attitude and motion sensing, the MPU6050 provides both acceleration and gyroscope raw data. The system uses acceleration values for tilt estimation and combines inter-sample acceleration changes with angular-velocity information for motion-disturbance judgment, thereby distinguishing between static tilt anomalies and dynamic impact disturbances. Considering the limited computing resources of the MCU platform, the system adopts lightweight linear conversion and threshold comparison methods instead of complex filtering or attitude-solving models. The tilt conversion relationship can be expressed as:

$$
\mathrm{tilt\_mdeg}=\frac{\mathrm{accel\_raw}\times 90000}{16384}
$$

This strategy enables the basic identification of manhole displacement, overturning, or external disturbance with low computational cost.

For environmental-state sensing, the MQ-2 simultaneously outputs analog AO and digital DO signals. The system uses AO to represent the trend of smoke-concentration variation and DO as a fast threshold-based alarm signal. To facilitate subsequent unified data processing, the system normalizes the AO raw value into a per-mille quantity:

$$
\mathrm{smoke\_permille}=\frac{\mathrm{smoke\_raw}\times 1000}{4095}
$$

This dual-channel design balances trend sensing and immediate response, making it suitable for edge-side warning in underground smoke or abnormal gas environments.

For spatial-state sensing, the GPS module provides coordinates, UTC time, and satellite count information. The system receives NMEA sentences through UART interrupts and parses key sentence types such as RMC and GGA. When positioning is valid, the terminal appends location and time information to the telemetry record. When positioning is invalid, the system still preserves the acquisition and upload capability of other state quantities, thereby avoiding interruption of the overall monitoring chain due to the failure of a single positioning module.

Overall, the key value of the multi-source sensing mechanism lies not in simply stacking more sensors, but in exploiting the complementarity among different physical quantities. The ultrasonic module describes underground water-level state, the MPU6050 describes attitude and disturbance, the MQ-2 describes the underground environment, and GPS describes spatial location. STM32F103 organizes these heterogeneous data on the edge side into structured inputs for subsequent anomaly judgment and platform-side processing.

### 3.2 Telemetry Transmission and Platform Coordination Mechanism
After completing terminal-side multi-source acquisition, the system must further solve the problem of how telemetry data are stably transmitted and how the platform performs coordinated processing. To balance prototype implementation cost, deployment flexibility, and development convenience, this study adopts ESP8266 to build a WiFi-based short-range wireless transmission scheme rather than wide-area communication methods such as NB-IoT or LoRa [22-23]. ESP8266 connects to the main controller through USART3 and completes network access, TCP connection establishment, and HTTP request transmission on the terminal side.

The system adopts a periodic telemetry-upload mechanism. The terminal organizes ultrasonic distance, water-level state, attitude and motion state, smoke state, and GPS information into unified query parameters and sends them to the backend through an HTTP GET interface. To keep the terminal-side control logic simple, the system uses time-slice scheduling based on a main loop rather than introducing an RTOS multi-task framework. GPS reception continues through UART interrupts, while the ultrasonic module, MPU6050, and MQ-2 complete sampling according to preset periods. WiFi reconnection and data upload are executed at longer intervals. Although this scheduling strategy is simple, it satisfies the real-time sampling, edge-side judgment, and periodic reporting requirements of the prototype system while avoiding the extra resource overhead associated with multi-task management.

On the platform side, the backend service is implemented using Node.js and the Fastify framework. The system provides a telemetry upload interface, a historical record query interface, a real-time event push interface, and a health-check interface. The backend first performs parameter validation and field normalization on the telemetry uploaded by the terminal, and then writes the data into corresponding JSONL files according to device identifiers, thereby supporting historical tracing and result replay in a lightweight prototype system. Compared with traditional approaches that only “receive and store” data, this backend additionally undertakes state-coordination functions, thus improving system continuity in real operation.

Among these functions, the GPS compensation mechanism is a key part of platform coordination. When the current upload does not contain newly valid positioning information but historical records for the device contain valid coordinates, the backend reuses the most recent valid position within a limited number of times and increments the lost-position count. Once the number of consecutive losses exceeds the threshold, the system explicitly marks the record as having no valid positioning information. This mechanism helps mitigate display jumps caused by short-term satellite loss, obstruction, or indoor conditions, thereby improving the continuity and interpretability of platform-side visualization.

For data distribution, the backend uses SSE to push newly arrived telemetry records to the frontend in real time, while the frontend, implemented with Vue 3, provides status overview, historical-data query, and anomaly display. In this way, the system forms a platform-coordination chain characterized by “periodic terminal upload, backend reception and processing, and real-time frontend update.” This mechanism supports not only single-device status tracking but also centralized monitoring and remote maintenance in multi-device scenarios.

### 3.3 Multi-Source Fusion Decision Model
On the basis of multi-source sensing and platform coordination, the system further constructs a fusion decision model for manhole scenarios. Different sensors reflect different types of risk characteristics, and a single sensing source can easily be affected by noise, installation conditions, or environmental changes, leading to false alarms or missed alarms [24]. Therefore, this paper adopts a lightweight rule-based fusion strategy on the edge side, incorporating distance, attitude, motion, smoke, and positioning information into a unified anomaly-decision framework.

According to the system design objectives, the terminal-side monitoring states can be summarized as follows: normal state, water-level anomaly state, tilt anomaly state, severe motion state, smoke anomaly state, and positioning-enabled upload state. Among them, water-level anomalies mainly correspond to excessive underground water accumulation or abrupt liquid-level changes; tilt and motion anomalies mainly correspond to manhole displacement, collision, or external damage; smoke anomalies correspond to underground fire or abnormal gas environments; and positioning information mainly supports remote asset localization and maintenance scheduling.

For formal description, let $D$ denote the ultrasonic distance value, $D_0$ the baseline distance under normal conditions, $T_x$ and $T_y$ the tilt quantities in the X and Y directions, $M$ the motion intensity, and $S$ the normalized smoke concentration. Then the sub-criteria can be defined as:

$$
R_d=
\begin{cases}
1, & |D-D_0|>T_d \\
0, & |D-D_0|\leq T_d
\end{cases}
$$

$$
R_t=
\begin{cases}
1, & |T_x|>T_{tx}\ \text{or}\ |T_y|>T_{ty} \\
0, & \text{otherwise}
\end{cases}
$$

$$
R_m=
\begin{cases}
1, & M>T_m \\
0, & M\leq T_m
\end{cases}
$$

$$
R_s=
\begin{cases}
1, & S>T_s\ \text{or}\ DO=1 \\
0, & \text{otherwise}
\end{cases}
$$

where $T_d$ denotes the water-level anomaly threshold, $T_{tx}$ and $T_{ty}$ denote the tilt thresholds, $T_m$ denotes the motion threshold, and $T_s$ denotes the smoke analog threshold. Accordingly, the comprehensive anomaly state can be expressed as:

$$
\mathrm{Alarm}=R_d \lor R_t \lor R_m \lor R_s
$$

The characteristics of this model are its simple structure, strong interpretability, and low computational cost, which make it suitable for deployment on resource-constrained embedded platforms such as STM32F103. At the same time, different sub-criteria correspond to different risk sources, allowing the platform side to further trace anomaly causes after an alarm is triggered rather than merely producing a single binary alarm result.

From the perspective of system operation logic, the terminal first calculates each sub-criterion on the edge side and uses the result to drive the local audio-visual alarm. Subsequently, the structured telemetry results are uploaded to the platform for historical storage, real-time display, and further analysis. In this way, the fusion decision model is not only the basis of terminal-side anomaly recognition, but also the semantic core of collaborative monitoring across the terminal, backend, and frontend. Although the model does not introduce complex machine-learning algorithms, it offers direct implementation, convenient debugging, and strong engineering practicality at the prototype stage, thereby providing a solid basis for future work on adaptive recognition and data-driven analysis.

## 4 Experiments and Result Analysis

### 4.1 Experimental Objectives
To verify the effectiveness and engineering applicability of the proposed system in manhole monitoring scenarios, this chapter analyzes the system through the presentation of the system prototype. Since the work in this paper is an engineering-oriented prototype implementation, the experiments and results should not only present the actual operating results of the terminal hardware and the frontend platform, but also show that the basic functions of the system can meet application requirements.

### 4.2 System Prototype and Frontend Presentation Results
To intuitively present the engineering implementation results of the system, this paper first provides the terminal hardware prototype and the operating interfaces of the frontend monitoring platform. Figure 5 shows the physical prototype of the terminal hardware. It can be observed that the system has completed the integrated connection of the STM32 main control board, the ESP8266 wireless module, the ultrasonic module, the GPS module, and other sensing units, thus providing the basic implementation conditions for terminal-side multi-source acquisition, edge processing, and wireless transmission.

Figure 6 shows the overview interface of the frontend monitoring platform. This interface takes the map as the core display area and combines top-level statistical cards with a right-side device-detail panel to centrally present the overall operating condition of the monitored manhole devices. The page can simultaneously display the total number of devices, the number of normal devices, warning devices, alarm devices, and offline devices, while also marking device locations on the map. In this way, managers can quickly grasp the overall monitoring status within the covered area. For a selected device, the interface can further present key information such as water distance, tilt angle, smoke state, latest update time, and positioning validity, thereby realizing a dual-level information organization mode of “global overview plus single-point inspection.”

Figure 7 shows the historical data query interface of the system. This interface organizes the uploaded records of multiple devices or a single device in tabular form and supports time-series review of water-level distance, attitude angle, smoke status, positioning strength, and geographic coordinates. Compared with the overview interface, which focuses more on real-time supervision, the historical query interface emphasizes data retention and process tracing. It is therefore useful for analyzing the variation trend of a given device over a continuous time period. For example, when underground water accumulation gradually increases or when the attitude angle of a device exhibits persistent fluctuations, the historical table can provide continuous data support for subsequent cause analysis and maintenance decision-making.

Figure 8 shows the abnormal-event display interface of the system. This interface centrally aggregates abnormal records such as warnings, alarms, and offline states, and provides the triggering cause for each event, such as smoke alarm, tilt warning, or water distance below a threshold. Compared with ordinary historical records, the abnormal-event interface directly highlights risk events together with their corresponding monitoring indicators, thereby helping maintenance personnel identify key targets more efficiently. Since the interface also preserves auxiliary fields such as timestamp, device identifier, water distance, tilt angle, and location status, it can support not only anomaly retrieval, but also subsequent event review and disposal-priority judgment.

Based on the above prototype and interface results, it can be concluded that the system has realized a complete operational closed loop from terminal acquisition to backend processing and frontend presentation. The hardware prototype figure reflects the multi-module integration capability on the terminal side, while the three frontend interfaces correspond to three typical service scenarios: real-time monitoring, historical tracing, and anomaly handling. On this basis, the paper further adopts a combination of itemized validation and system joint debugging experiments to evaluate the actual performance of the system in water-level measurement and real-time response.

## 5 Conclusion
This paper addresses the safety-monitoring requirements of smart city manhole facilities and designs and implements a multidimensional smart manhole monitoring system based on STM32F103. The system integrates ultrasonic water-level measurement, attitude and motion sensing, smoke detection, GPS positioning, ESP8266 wireless communication, and an audio-visual alarm unit, thereby establishing a complete technical chain from state acquisition and edge-side decision-making to local warning and remote upload. Through the analysis of the overall system architecture, key implementation technologies, and the multi-source fusion decision mechanism, this paper completes the design of a multidimensional sensing and monitoring scheme for manhole scenarios.

The experimental results indicate that the system can stably perform ultrasonic water-level measurement, attitude recognition, smoke detection, positioning analysis, and wireless telemetry upload, and can comprehensively perceive and collaboratively judge underground water-level variation, tilt disturbance, abnormal motion, and smoke state. Compared with single-sensor monitoring approaches, the proposed system exhibits more significant advantages in monitoring dimensions, on-site response capability, and information completeness, thereby verifying the feasibility and engineering value of the multi-source sensing strategy in smart manhole monitoring scenarios.

Although the system has realized multidimensional sensing and remote monitoring of manhole conditions, there is still room for further improvement. On the one hand, the current anomaly-decision mechanism is mainly based on threshold rules. In future work, more measured data may be incorporated to introduce adaptive calibration or data-driven methods so as to improve robustness in complex scenarios [2-3,9-10,24]. On the other hand, the system still has research potential in low-power management, long-term deployment stability, and platform-side intelligent analysis capability [5-6,20,22-23]. Future work may focus on terminal energy optimization, anomaly-recognition model improvement, and enhancement of platform coordination mechanisms, thereby further improving the practical application performance of the system in urban infrastructure monitoring.

## References
[1] Guo H. Research on Ecological Design of Intelligent Manhole Covers Based on Fuzzy Analytic Hierarchy Process[J]. Sustainability, 2024, 16(13): 5310-5310.

[2] Guo J, Wang K, Sun J, et al. Research and Implementation of Low-Power Anomaly Recognition Method for Intelligent Manhole Covers[J]. Electronics, 2023, 12(8): 1926-1938.

[3] Li C, Gu S, Guo G, et al. Alarm method of communication intelligent manhole cover based on multiple event fusion[J]. EURASIP Journal on Wireless Communications and Networking, 2021, 2021(1): 1-15.

[4] Islam M D S, Saha T, Mir M D S, et al. Design and Implementation of IoT-Based Manhole Monitoring System[C]//2024 IEEE International Conference on Power, Electrical, Electronics and Industrial Applications (PEEIACON). Rajshahi, Bangladesh: IEEE, 2024: 502-507.

[5] Guo X, Liu B, Wang L. Design and implementation of intelligent manhole cover monitoring system based on NB-IoT[C]//2019 International Conference on Robots and Intelligent System (ICRIS). Haikou, China: IEEE, 2019: 207-210.

[6] Zhang J, Zeng X. Design of intelligent manhole cover monitoring system based on narrow band Internet of things[C]//2022 7th International Conference on Intelligent Computing and Signal Processing (ICSP). Xi'an, China: IEEE, 2022: 1354-1357.

[7] Ganesan P, Supraja N, Praveena R. IoT based manhole detection and monitoring system[C]//AIP Conference Proceedings. Melville, NY: AIP Publishing, 2023, 2725(1): 070001.

[8] Ravi Kumar K, Vijaya Lakshmi K, Rohin Kumar G, et al. Smart Manhole Monitoring System[C]//2023 5th International Conference on Smart Systems and Inventive Technology (ICSSIT). Tirunelveli, India: IEEE, 2023: 475-481.

[9] Mohanraj S, Aravindh R, Arshath Ahamed N, et al. IoT Based System for Manhole Monitoring and Management[C]//2023 9th International Conference on Advanced Computing and Communication Systems (ICACCS). Coimbatore, India: IEEE, 2023: 1357-1361.

[10] Liang Y, Chen L, Xu B. Design of Intelligent Management System for Manhole Cover[C]//2022 IEEE 5th Advanced Information Management, Communicates, Electronic and Automation Control Conference (IMCEC). Chongqing, China: IEEE, 2022: 130-134.

[11] Rasheed W M, Abdulla R, San L Y. Manhole cover monitoring system over IOT[J]. Journal of Applied Technology and Innovation, 2021, 5(3): 1-6.

[12] Aly H H, Soliman A H, Mouniri M. Towards a Fully Automated Monitoring System for Manhole Cover: Smart Cities and IoT Applications[C]//2015 IEEE First International Smart Cities Conference (ISC2). Guadalajara, Mexico: IEEE, 2015: 24-30.

[13] Mo F, Yu L, Zhang Z, et al. Design and Implementation of Manhole Cover Safety Monitoring System Based on Smart Light Pole[J]. Mathematical Problems in Engineering, 2022, 2022: 3081649.

[14] Nallamothu V K, Medidi S, Jannu S P. IoT based Manhole Detection and Monitoring System[C]//2022 IEEE International Conference on Distributed Computing and Electrical Circuits and Electronics (ICDCECE). Ballari, India: IEEE, 2022: 1-6.

[15] Yu L, Zhang Z, Lai Y, et al. Edge Computing-Based Intelligent Monitoring System for Manhole Cover[J]. Mathematical Biosciences and Engineering, 2023, 20(10): 18792-18819.

[16] Imran M A, Swapno S M M R, Chhabra G, et al. IoT-Enabled Smart Manhole Management System for Real-time Status, Water Level, and Gas Detection[C]//2024 International Conference on Intelligent Systems for Cybersecurity (ISCS). IEEE, 2024: 1-7.

[17] Sebicho S W, Lou B, Anito B S. A Multi-Parameter Flexible Smart Water Gauge for the Accurate Monitoring of Urban Water Levels and Flow Rates[J]. Eng, 2024, 5(1): 198-216.

[18] Salehin S, Akter S S, Ibnat A, et al. An IoT Based Proposed System for Monitoring Manhole in Context of Bangladesh[C]//2018 4th International Conference on Electrical Engineering and Information & Communication Technology (iCEEiCT). Dhaka, Bangladesh: IEEE, 2018: 411-415.

[19] Nataraja N, Amruthavarshini R, Chaitra N L, et al. Secure Manhole Monitoring System Employing Sensors and GSM Techniques[C]//2018 3rd IEEE International Conference on Recent Trends in Electronics, Information and Communication Technology (RTEICT). Bangalore, India: IEEE, 2018: 2078-2082.

[20] Xie Y, Wang H, Liu J, et al. On a Working Monitoring System of Manhole Wells Based on Technology of Internet of Things[C]//2021 6th International Conference on Intelligent Computing and Signal Processing (ICSP). Xi'an, China: IEEE, 2021: 1452-1455.

[21] Alavi A H, Jiao P, Buttlar W G, et al. Internet of Things-enabled smart cities: State-of-the-art and future trends[J]. Measurement, 2018, 129: 589-606.

[22] Mekki K, Bajic E, Chaxel F, et al. A comparative study of LPWAN technologies for large-scale IoT deployment[J]. ICT Express, 2019, 5(1): 1-7.

[23] Ogbodo E U, Abu-Mahfouz A M, Kurien A M. A Survey on 5G and LPWAN-IoT for Improved Smart Cities and Remote Area Applications: From the Aspect of Architecture and Security[J]. Sensors, 2022, 22(16): 6313.

[24] DeMedeiros K, Hendawi A, Alvarez M. A Survey of AI-Based Anomaly Detection in IoT and Sensor Networks[J]. Sensors, 2023, 23(3): 1352.
