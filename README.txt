����STM32F0xxϵ�е�Flash bootlader��Application���̣�
��STM32F1��F4ϵ�л�����һ��������ġ�

STM32F0ϵ����Ҫ��Application����ǰ��������´���
main����
{
	memcpy((uint32_t*)0x20000000, (uint32_t*)0x08004000, VECTOR_SIZE); 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//��ʱ�ӣ������п��ܲ��ܳɹ������ж�
	SYSCFG_MemoryRemapConfig(SYSCFG_MemoryRemap_SRAM); 
}

�˹��̻���Ҫ��Keil�������Ӧ�����á�