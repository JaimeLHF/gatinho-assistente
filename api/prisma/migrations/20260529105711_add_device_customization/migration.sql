-- CreateTable
CREATE TABLE `device_customizations` (
    `id` VARCHAR(191) NOT NULL,
    `deviceId` VARCHAR(191) NOT NULL,
    `body` VARCHAR(191) NOT NULL DEFAULT '#D4A574',
    `stripes` VARCHAR(191) NOT NULL DEFAULT '#8B5E3C',
    `belly` VARCHAR(191) NOT NULL DEFAULT '#F5D4A0',
    `outline` VARCHAR(191) NOT NULL DEFAULT '#000000',
    `eyes` VARCHAR(191) NOT NULL DEFAULT '#FFFFFF',
    `nose` VARCHAR(191) NOT NULL DEFAULT '#FF9B9B',

    UNIQUE INDEX `device_customizations_deviceId_key`(`deviceId`),
    PRIMARY KEY (`id`)
) DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- AddForeignKey
ALTER TABLE `device_customizations` ADD CONSTRAINT `device_customizations_deviceId_fkey` FOREIGN KEY (`deviceId`) REFERENCES `devices`(`id`) ON DELETE CASCADE ON UPDATE CASCADE;
