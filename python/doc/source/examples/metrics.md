---
file_format: mystnb
kernelspec:
  name: python3
---

# Computing unary quality metrics
## Read data sets

```{code-cell}
import moocore as moo
spherical = moo.read_datasets("spherical-250-10-3d.txt")
uniform = moo.read_datasets("uniform-250-10-3d.txt")
print(spherical.shape)
print(uniform.shape)
```
