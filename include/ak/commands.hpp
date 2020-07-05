#pragma once
#include <ak/ak.hpp>

namespace ak
{
	class vk_given_state {};
	class vk_barrier {};
	class vk_command {};
	
	class commands
	{
		using CT = std::variant<
			vk_given_state,
			vk_barrier,
			vk_command,
			std::function<void(ak::command_buffer_t&)>
		>;
		
	public:
		commands() = default;
		commands(commands&&) noexcept = default;
		commands(const commands&) = delete;
		commands& operator=(commands&&) noexcept = default;
		commands& operator=(const commands&) = delete;
		~commands() = default;

		void add(vk_given_state aGivenState) { mRecordedCommands.push_back(std::move(aGivenState)); }
		void add(vk_barrier aBarrier) { mRecordedCommands.push_back(std::move(aBarrier)); }
		void add(vk_command aCommand) { mRecordedCommands.push_back(std::move(aCommand)); }
		template <typename F>
		void add(F&& aCustomFunctionality) { mRecordedCommands.emplace_back(std::forward(aCustomFunctionality)); }

		void record_into(command_buffer_t& aCommandBuffer)
		{
			for (auto& c : mRecordedCommands) {
				if (std::holds_alternative<vk_given_state>(c)) {
					continue;
				}
				if (std::holds_alternative<vk_barrier>(c)) {
					// TODO: If it is an auto-barrier => find stage masks and access masks!
					continue;
				}
				if (std::holds_alternative<vk_command>(c)) {
					// TODO: Add command to aCommandBuffer
					continue;
				}
				if (std::holds_alternative<std::function<void(ak::command_buffer_t&)>>(c)) {
					std::get<std::function<void(ak::command_buffer_t&)>>(c)(aCommandBuffer);
				}
			}
		}
		
	private:
		std::vector<CT> mRecordedCommands;
	};
}
