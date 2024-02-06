#include "render/commands.hpp"

using namespace sm;
using namespace sm::render;

void IRenderPass::add_input(IPassInput *input) {
    m_inputs.push_back(input);
}

void IRenderPass::add_output(IPassOutput *output) {
    m_outputs.push_back(output);
}

void IRenderPass::build() {
    for (auto input : m_inputs) {
        input->update_input(m_context);
    }

    execute();

    for (auto output : m_outputs) {
        output->update_output(m_context);
    }
}
