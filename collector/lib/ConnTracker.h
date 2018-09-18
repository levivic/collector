/** collector

A full notice with attributions is provided along with this source code.

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

* In addition, as a special exception, the copyright holders give
* permission to link the code of portions of this program with the
* OpenSSL library under certain conditions as described in each
* individual source file, and distribute linked combinations
* including the two.
* You must obey the GNU General Public License in all respects
* for all of the code used other than OpenSSL.  If you modify
* file(s) with this exception, you may extend this exception to your
* version of the file(s), but you are not obligated to do so.  If you
* do not wish to do so, delete this exception statement from your
* version.
*/

#ifndef COLLECTOR_CONNTRACKER_H
#define COLLECTOR_CONNTRACKER_H

#include <mutex>
#include <vector>

#include "Hash.h"
#include "NetworkConnection.h"

namespace collector {

// ConnStatus encapsulates the status of a connection, comprised of the timestamp when the connection was last seen
// alive (in microseconds since epoch), and a flag indicating whether the connection is currently active.
class ConnStatus {
 public:
  ConnStatus() : data_(0UL) {}
  ConnStatus(int64_t microtimestamp, bool active) : data_((microtimestamp & ~0x1UL) | ((active) ? 0x1 : 0x0)) {}

  int64_t LastActiveTime() const { return data_ & ~0x1UL; }
  bool IsActive() const { return data_ & 0x1UL; }

  void SetActive(bool active) {
    if (active) data_ |= 0x1UL;
    else data_ &= ~0x1UL;
  }

  ConnStatus WithStatus(bool active) const {
    int64_t new_data = data_;
    if (active) new_data |= 0x1UL;
    else new_data &= ~0x1UL;
    return ConnStatus(new_data);
  }

  bool operator==(const ConnStatus& other) const {
    return data_ == other.data_;
  }

  bool operator!=(const ConnStatus& other) const {
    return !(*this == other);
  }

 private:
  explicit ConnStatus(int64_t data) : data_(data) {}

  int64_t data_;
};

using ConnMap = UnorderedMap<Connection, ConnStatus>;

class ConnectionTracker {
 public:
  void AddConnection(const Connection& conn, int64_t timestamp);
  void RemoveConnection(const Connection& conn, int64_t timestamp);

  void Update(const std::vector<Connection>& all_conns, int64_t timestamp);

  // Atomically fetch a snapshot of the current state, removing all inactive connections if requested.
  ConnMap FetchState(bool clear_inactive = true);

  // ComputeDelta computes a diff between new_state and *old_state, and stores the diff in *old_state.
  static void ComputeDelta(const ConnMap& new_state, ConnMap* old_state);

 private:
  // Emplace a connection into the state ConnMap, or update its timestamp if the supplied timestamp is more recent
  // than the stored one.
  void EmplaceOrUpdateNoLock(const Connection& conn, ConnStatus status);

  std::mutex mutex_;
  ConnMap state_;
};

}  // namespace collector

#endif //COLLECTOR_CONNTRACKER_H
