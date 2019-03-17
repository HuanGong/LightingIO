#ifndef _COMPONENT_LOG_METRICS_H_
#define _COMPONENT_LOG_METRICS_H_

#include <vector>
#include <atomic>
#include <thread>
#include "metrics_item.h"
#include "metrics_define.h"
#include "metrics_container.h"

namespace component {

typedef std::shared_ptr<MetricContainer> RefMetricContainer;

class StashQueueGroup {
public:
  StashQueueGroup(uint32_t count);
  uint32_t GroupSize() const {return queues_.size();}
  StashQueue& At(uint32_t idx) {return queues_.at(idx);};
  StashQueue& NextQueue() {return queues_[++index_ % queues_.size()];}
private:
  std::atomic<uint32_t> index_;
  std::vector<StashQueue> queues_;
};

class MetricsStash : public MetricContainer::Provider {
public:
  typedef struct Handler {
    /* handle this metric, can do other things like prometheus monitor, or just by
     * judge the thread presure by check the metric birth time
     * return true mean metric had been handled, MetricStash will not handle again
     * the stash will continue sumup this metric when return false;
     * */
    virtual bool HandleMetric(const MetricsItem* item) {return false;};
  } Handler;

  void RegistDistArgs(const std::string&, const MetricDistArgs);

  MetricsStash();
  void SetHandler(Handler* d);
  void SetReportInterval(uint32_t interval);

  void Start();
  void StopSync();

  bool Stash(MetricsItemPtr&& item);
  const MetricDistArgs* GetDistArgs(const std::string& name) override;
private:
  void StashMain();
  void GenerateReport();
  uint64_t ProcessMetric();

  bool running_;
  std::thread* th_;
  uint32_t report_interval_; //in second
  uint32_t last_report_time_; //timestamp in second
  Handler* handler_ = nullptr;
  StashQueueGroup queue_groups_;
  RefMetricContainer container_;
  std::unordered_map<std::string, MetricDistArgs> dist_args_map_;
};

}
#endif
