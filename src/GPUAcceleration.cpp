#include "gaden/internal/GPUAcceleration.hpp"
#include "boost/compute.hpp"
#include "gaden/core/Logging.hpp"
#include <boost/compute/container/vector.hpp>
#include <boost/compute/types/struct.hpp>

namespace compute = boost::compute;

namespace gaden
{
    struct Data
    {
        int x;
    };

    struct Point
    {
        int x;
        Data data;
    };
} // namespace gaden

BOOST_COMPUTE_ADAPT_STRUCT(gaden::Data, Data, (x));
BOOST_COMPUTE_ADAPT_STRUCT(gaden::Point, Point, (x, data));

namespace gaden
{
    std::optional<compute::kernel> test;

    void GPUSetup()
    {
        compute::device gpu = compute::system::default_device();
        compute::context context(gpu);
        compute::command_queue queue(context, gpu);

        std::string source = BOOST_COMPUTE_STRINGIZE_SOURCE(
            int add_four(int x) {
                return x + 4;
            }

            __kernel void test(global Point * values, global Point * results) {
                int i = get_global_id(0);
                results[i].data.x = add_four(values[i].x);
            });

        source = compute::type_definition<Point>() + "\n" + source;
        source = compute::type_definition<Data>() + "\n" + source;

        compute::program program = compute::program::create_with_source(source, context);
        program.build();

        test = program.create_kernel("test");

        compute::vector<Point> a(100, {0, 0}, queue);
        compute::vector<Point> b(100, {0, 0}, queue);

        test->set_arg(0, a.get_buffer());
        test->set_arg(1, b.get_buffer());

        queue.enqueue_1d_range_kernel(*test, 0, 100, 0);
        std::vector<Point> results(100);
        compute::copy(b.begin(), b.end(), results.begin(), queue);

        queue.finish();
    }

    void GPURun()
    {
    }
} // namespace gaden