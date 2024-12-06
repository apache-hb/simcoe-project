#include <private/internal-components.h>
static const struct hwloc_component * hwloc_static_components[] = {
  &hwloc_noos_component,
  &hwloc_xml_component,
  &hwloc_synthetic_component,
  &hwloc_xml_nolibxml_component,
  &hwloc_xml_libxml_component,
#ifdef __linux__
  &hwloc_linux_component,
#endif
#ifdef _WIN32
  &hwloc_windows_component,
#endif
  &hwloc_x86_component,
  NULL
};
