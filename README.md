
LigthingIO is a 'light' network IO application framework;

- base code
- message loop
- coroutine scheduler
- lazyinstance from chromium
- net io based on loop
- raw protocol
- line protocol
- http 1.x protocol
- async redis client side support
- thread safe lru component

performance:
---
一般业务场景下IO性能: 无论是否使用coro调度器，性能约等于Nginx, 如果你要说Nginx好，你觉得有意义就行.
复杂业务，使用coroutine调度器的情况下，性能肯定不会差.
通用计算型任务；除了coroutine task外, 提供normal task的支持

NOTE
---
  INPROCESS, 不保证没有bug；

  This library still in progress, do not use it in your project or production environments;
  **email** me if any question and problem;

Learning and integrate what i think into LightingIO

LightingIO itself just a normal network application framework; all its technology from my practice and work

at the start time of this project; i use libevent, but i find many things was limited by the exsiting code; it's hard to hack or change
the libevent code; so... I give up libvent at last, and then start all things from zero, in thoes days and night, thanks the support from
my girl friends; and other reference books


From HuanGong 2018-01-08
