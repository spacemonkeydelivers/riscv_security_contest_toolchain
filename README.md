# Overview

Custom toolchain for RISC-V security contest.

The patches to vanilla gcc include:

- a patch to `binutils` to allow new instructions "st", "lt"
(store tag, load tag). The path also introduces a new CSR: **tags**
- a patch to `binutils` introducing new CSR: **rnd**
- a patch to `newlib` to modify malloc/free routines to returned tagged addresses.

Note regarding tag generation. Our [SOC](https://github.com/spacemonkeydelivers/riscv_security_contest_project)
supports HW RNG via **rnd** csr. We can adjust our implementation to
use HW pseudo-random generator instead of a simple increment of a tag value.
