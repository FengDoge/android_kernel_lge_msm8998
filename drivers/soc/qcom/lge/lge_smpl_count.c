/*
 * driver/power/lge_smpl_count.c
 *
 * Copyright (C) 2013 LGE, Inc
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/input/qpnp-power-on.h>
#include <soc/qcom/smem.h>

#define MODULE_NAME "lge_smpl_count"
#define PWR_ON_EVENT_KEYPAD           0x80
#define PWR_ON_EVENT_CABLE            0x40
#define PWR_ON_EVENT_PON1             0x20
#define PWR_ON_EVENT_USB              0x10
#define PWR_ON_EVENT_DC               0x08
#define PWR_ON_EVENT_RTC              0x04
#define PWR_ON_EVENT_SMPL             0x02
#define PWR_ON_EVENT_HARD_RESET       0x01
#define PWR_OFF_EVENT_S2_RESET        5
#define PWR_OFF_EVENT_S3_RESET        39

static int dummy_arg;

struct lge_smpl_count_data {
	uint32_t smpl_boot;
};

static struct lge_smpl_count_data *data;

uint16_t *poweron_st = NULL;
uint16_t power_on_status_info_get(void)
{
	poweron_st = smem_alloc(SMEM_POWER_ON_STATUS_INFO, sizeof(poweron_st)*2,
							0, SMEM_ANY_HOST_FLAG);

	if (poweron_st == NULL)
		return -1;

	return *poweron_st;
}

static int read_smpl_count(char *buffer, const struct kernel_param *kp)
{
	uint16_t boot_cause;
	int warm_reset, poff_sts;

	boot_cause = power_on_status_info_get();
	warm_reset = qpnp_pon_is_warm_reset();
	poff_sts = qpnp_pon_is_off_reason();
	pr_info("[BOOT_CAUSE] PON:0x%2x, warm_reset:%d, POFF:%d\n",
			boot_cause, warm_reset, poff_sts);

	if (data == NULL){
		pr_err("lge_smpl_count_data is NULL\n");
		return -1;
	}
#ifdef CONFIG_MACH_MSM8998_JOAN_VZW
	if ((warm_reset == 0)
			&& ((poff_sts == PWR_OFF_EVENT_S3_RESET)
			|| (poff_sts == PWR_OFF_EVENT_S2_RESET)
			|| (boot_cause &= PWR_ON_EVENT_SMPL))) {
#else
	if ((boot_cause &= PWR_ON_EVENT_SMPL) && (warm_reset == 0)) {
#endif
		pr_info("[SMPL_CNT] ===> is smpl boot\n");
		data->smpl_boot = 1;
	} else {
		pr_info("[SMPL_CNT] ===> not smpl boot!!!!!\n");
		data->smpl_boot = 0;
	}

	return sprintf(buffer, "%d", data->smpl_boot);
}
module_param_call(smpl_boot, NULL, read_smpl_count, &dummy_arg,
		S_IWUSR | S_IRUGO);


static int lge_smpl_count_probe(struct platform_device *pdev)
{
	data = kmalloc(sizeof(struct lge_smpl_count_data),
			GFP_KERNEL);
	if (data == NULL) {
		pr_err("%s: No memory\n", __func__);
		return -ENOMEM;
	}
	return 0;
}

static int lge_smpl_count_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver lge_smpl_count_driver = {
	.probe = lge_smpl_count_probe,
	.remove = lge_smpl_count_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static struct platform_device lge_smpl_count_device = {
	.name = MODULE_NAME,
	.dev = {
		.platform_data = NULL,
	}
};

static int __init lge_smpl_count_init(void)
{
	platform_device_register(&lge_smpl_count_device);

	return platform_driver_register(&lge_smpl_count_driver);
}

static void __exit lge_smpl_count_exit(void)
{
	if (data != NULL)
		kfree(data);

	platform_driver_unregister(&lge_smpl_count_driver);
}

module_init(lge_smpl_count_init);
module_exit(lge_smpl_count_exit);

MODULE_DESCRIPTION("LGE smpl_count");
MODULE_LICENSE("GPL");

