# Kernel file system lab

Implement a simple in-memory filesystem.

[doc](https://www.kernel.org/doc/Documentation/filesystems/vfs.rst)

## Step 1. Build Kernel Module

In project root directory:

```shell=
make
```
---

## Step 2. Install Kernel Module

In project root directory:

```shell=
sudo insmod kmod/labfs.ko
```

---

## Step 3. Mount directory into labfs

```shell=
sudo mount -t labfs nodev test
```

---

## Step 4. Do some file operations

```shell=
sudo mkdir -p test/xxx
sudo ls test
sudo rm -r test/xxx
sudo ls test
```
