import jt;

void check() {
#if defined(JT_CHECK_MESSAGE)
  (void)sizeof(jt::log::message);
#elif defined(JT_CHECK_SERVICE_IMPL)
  (void)sizeof(jt::log::service_impl);
#elif defined(JT_CHECK_DEFAULT_FORMATTER)
  (void)sizeof(jt::log::default_formatter);
#elif defined(JT_CHECK_TID)
  (void)jt::detail::tid();
#elif defined(JT_CHECK_INTRUSIVE_MPSC)
  struct node {
    node* next;
  };
  jt::detail::intrusive_mpsc_queue<&node::next> q;
  (void)q;
#elif defined(JT_CHECK_STRING)
  jt::detail::string s;
  (void)s;
#elif defined(JT_CHECK_DEQUE)
  jt::detail::deque<int> d;
  (void)d;
#elif defined(JT_CHECK_UNORDERED_MAP)
  jt::detail::unordered_map<int, int> m;
  (void)m;
#elif defined(JT_CHECK_CPU_PAUSE)
  jt::detail::cpu_pause();
#elif defined(JT_CHECK_METRIC_VALUE)
  (void)sizeof(jt::detail::metric_value);
#else
#error "JT_CHECK_* is not defined"
#endif
}
