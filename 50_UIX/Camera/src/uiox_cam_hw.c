#include "uiox_cam_hw.h"
#include <string.h>
#include <errno.h>

int uiox_cam_hw_init(uiox_cam_hw_t *hw, const uiox_cam_hw_ops_t *ops)
{
    if (!hw || !ops || !ops->init) return -EINVAL;
    hw->priv = (void *)ops;
    return ops->init(hw);
}

int uiox_cam_hw_start(uiox_cam_hw_t *hw)
{
    if (!hw || !hw->priv) return -EINVAL;
    const uiox_cam_hw_ops_t *ops = (const uiox_cam_hw_ops_t *)hw->priv;
    return ops->start ? ops->start(hw) : 0;
}

void uiox_cam_hw_stop(uiox_cam_hw_t *hw)
{
    if (!hw || !hw->priv) return;
    const uiox_cam_hw_ops_t *ops = (const uiox_cam_hw_ops_t *)hw->priv;
    if (ops->stop) ops->stop(hw);
}
