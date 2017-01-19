#pragma once

namespace sol
{
class state;
template <bool, typename> class basic_table_core;
using table = basic_table_core<false, class reference>;
}
