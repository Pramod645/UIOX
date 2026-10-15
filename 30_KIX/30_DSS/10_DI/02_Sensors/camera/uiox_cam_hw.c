#include "uiox_cam_hw.h"

int uiox_cam_hw_init(uiox_cam_hw_t *hw, const uiox_cam_hw_ops_t *ops)
{
    if (!hw || !ops || !ops->init) return -EINVAL;
    hw->ops = (void *)ops;
    return ops->init(hw);
}

int uiox_cam_hw_start(uiox_cam_hw_t *hw)
{
    if (!hw || !hw->ops) return -EINVAL;
    const uiox_cam_hw_ops_t *ops = (const uiox_cam_hw_ops_t *)hw->ops;
    return ops->start ? ops->start(hw) : 0;
}

void uiox_cam_hw_stop(uiox_cam_hw_t *hw)
{
    if (!hw || !hw->ops) return;
    const uiox_cam_hw_ops_t *ops = (const uiox_cam_hw_ops_t *)hw->ops;
    if (ops->stop) ops->stop(hw);
}
