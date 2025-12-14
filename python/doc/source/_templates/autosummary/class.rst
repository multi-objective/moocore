.. raw:: html

   <div class="prename">{{ module }}.</div>
   <div class="empty"></div>

{{ objname | escape | underline}}

.. currentmodule:: {{ module }}

.. autoclass:: {{ objname }}
   :members:
   :inherited-members:
   :special-members: __call__

.. minigallery:: {{ module }}.{{ objname }} {% for meth in methods %}{{ module }}.{{ objname }}.{{ meth }} {% endfor %}
   :add-heading: Gallery examples
   :heading-level: -
